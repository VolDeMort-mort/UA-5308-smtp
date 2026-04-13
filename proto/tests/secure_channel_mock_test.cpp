#include <arpa/inet.h>
#include <cstring>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <sodium.h>
#include <string>

#include "ClientSecureChannel.hpp"
#include "IConnection.hpp"
#include "ServerSecureChannel.hpp"

using ::testing::_;
using ::testing::DoAll;
using ::testing::Invoke;
using ::testing::Return;

class MockConnection : public IConnection
{
public:
	MOCK_METHOD(bool, Send, (const std::string& data), (override));
	MOCK_METHOD(bool, Receive, (std::string& out_data), (override));
	MOCK_METHOD(bool, SendRaw, (const unsigned char* buffer, std::size_t size), (override));
	MOCK_METHOD(bool, ReceiveRaw, (unsigned char* buffer, std::size_t size), (override));
	MOCK_METHOD(bool, IsOpen, (), (const, noexcept, override));
	MOCK_METHOD(bool, Close, (), (override));
};

class TestableServer : public ServerSecureChannel
{
public:
	explicit TestableServer(IConnection& conn) : ServerSecureChannel(conn) {}
	void forceSecure() { m_secure = true; }
};

class TestableClient : public ClientSecureChannel
{
public:
	explicit TestableClient(IConnection& conn) : ClientSecureChannel(conn) {}
	void forceSecure() { m_secure = true; }
};

TEST(SecureChannelMockTest, SendNotSecure_ConnSendFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);

	EXPECT_CALL(mockConn, Send(_)).WillOnce(Return(false));
	EXPECT_FALSE(channel.Send("hello"));
}

TEST(SecureChannelMockTest, SendNotSecure_ConnSendSucceeds)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);

	EXPECT_CALL(mockConn, Send(_)).WillOnce(Return(true));
	EXPECT_TRUE(channel.Send("hello"));
}

TEST(SecureChannelMockTest, ReceiveNotSecure_ConnReceiveFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);

	EXPECT_CALL(mockConn, Receive(_)).WillOnce(Return(false));
	std::string out;
	EXPECT_FALSE(channel.Receive(out));
}

TEST(SecureChannelMockTest, SendSecure_LengthSendRawFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);
	channel.forceSecure();

	EXPECT_CALL(mockConn, SendRaw(_, sizeof(uint32_t))).WillOnce(Return(false));
	EXPECT_FALSE(channel.Send("hello"));
}

TEST(SecureChannelMockTest, SendSecure_DataSendRawFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);
	channel.forceSecure();

	::testing::InSequence seq;
	EXPECT_CALL(mockConn, SendRaw(_, sizeof(uint32_t))).WillOnce(Return(true));
	EXPECT_CALL(mockConn, SendRaw(_, _)).WillOnce(Return(false));
	EXPECT_FALSE(channel.Send("hello"));
}

TEST(SecureChannelMockTest, ReceiveSecure_LengthReceiveRawFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);
	channel.forceSecure();

	EXPECT_CALL(mockConn, ReceiveRaw(_, sizeof(uint32_t))).WillOnce(Return(false));
	std::string out;
	EXPECT_FALSE(channel.Receive(out));
}

TEST(SecureChannelMockTest, ReceiveSecure_ZeroLength)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);
	channel.forceSecure();

	EXPECT_CALL(mockConn, ReceiveRaw(_, sizeof(uint32_t)))
		.WillOnce(Invoke([](unsigned char* buf, std::size_t) {
			uint32_t zero = 0;
			std::memcpy(buf, &zero, sizeof(zero));
			return true;
		}));
	std::string out;
	EXPECT_FALSE(channel.Receive(out));
}

TEST(SecureChannelMockTest, ReceiveSecure_DataReceiveRawFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);
	channel.forceSecure();

	const uint32_t dataLen = 64;
	const uint32_t netLen = htonl(dataLen);

	::testing::InSequence seq;
	EXPECT_CALL(mockConn, ReceiveRaw(_, sizeof(uint32_t)))
		.WillOnce(Invoke([&](unsigned char* buf, std::size_t) {
			std::memcpy(buf, &netLen, sizeof(netLen));
			return true;
		}));
	EXPECT_CALL(mockConn, ReceiveRaw(_, dataLen)).WillOnce(Return(false));
	std::string out;
	EXPECT_FALSE(channel.Receive(out));
}

TEST(SecureChannelMockTest, ReceiveSecure_BadDecrypt)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);
	channel.forceSecure();

	const std::size_t nonce_sz = crypto_aead_chacha20poly1305_ietf_NPUBBYTES;
	const std::size_t tag_sz   = crypto_aead_chacha20poly1305_ietf_ABYTES;
	const uint32_t dataLen     = static_cast<uint32_t>(nonce_sz + tag_sz + 1);
	const uint32_t netLen      = htonl(dataLen);

	::testing::InSequence seq;
	EXPECT_CALL(mockConn, ReceiveRaw(_, sizeof(uint32_t)))
		.WillOnce(Invoke([&](unsigned char* buf, std::size_t) {
			std::memcpy(buf, &netLen, sizeof(netLen));
			return true;
		}));
	EXPECT_CALL(mockConn, ReceiveRaw(_, dataLen))
		.WillOnce(Invoke([](unsigned char* buf, std::size_t size) {
			std::memset(buf, 0xFF, size);
			return true;
		}));
	std::string out;
	EXPECT_FALSE(channel.Receive(out));
}

TEST(SecureChannelMockTest, ServerStartTLS_ReceiveKeyFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);

	EXPECT_CALL(mockConn, ReceiveRaw(_, crypto_kx_PUBLICKEYBYTES)).WillOnce(Return(false));
	EXPECT_FALSE(channel.StartTLS());
}

TEST(SecureChannelMockTest, ServerStartTLS_SendKeyFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableServer channel(mockConn);

	unsigned char clientPub[crypto_kx_PUBLICKEYBYTES];
	unsigned char clientPriv[crypto_kx_SECRETKEYBYTES];
	crypto_kx_keypair(clientPub, clientPriv);

	EXPECT_CALL(mockConn, ReceiveRaw(_, crypto_kx_PUBLICKEYBYTES))
		.WillOnce(Invoke([&](unsigned char* buf, std::size_t size) {
			std::memcpy(buf, clientPub, size);
			return true;
		}));
	EXPECT_CALL(mockConn, SendRaw(_, crypto_kx_PUBLICKEYBYTES)).WillOnce(Return(false));
	EXPECT_FALSE(channel.StartTLS());
}

TEST(SecureChannelMockTest, ClientStartTLS_SendKeyFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableClient channel(mockConn);

	EXPECT_CALL(mockConn, SendRaw(_, crypto_kx_PUBLICKEYBYTES)).WillOnce(Return(false));
	EXPECT_FALSE(channel.StartTLS());
}

TEST(SecureChannelMockTest, ClientStartTLS_ReceiveKeyFails)
{
	::testing::NiceMock<MockConnection> mockConn;
	TestableClient channel(mockConn);

	EXPECT_CALL(mockConn, SendRaw(_, crypto_kx_PUBLICKEYBYTES)).WillOnce(Return(true));
	EXPECT_CALL(mockConn, ReceiveRaw(_, crypto_kx_PUBLICKEYBYTES)).WillOnce(Return(false));
	EXPECT_FALSE(channel.StartTLS());
}
