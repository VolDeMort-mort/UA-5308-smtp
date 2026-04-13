#include "SocketAcceptor.hpp"
#include "SocketConnector.hpp"
#include "SocketConnection.hpp"
#include "ServerSecureChannel.hpp"
#include "ClientSecureChannel.hpp"
#include "Logger.h"
#include "ConsoleStrategy.h"

#include <gtest/gtest.h>
#include <sodium.h>
#include <unistd.h>

using boost::asio::ip::tcp;

static uint16_t channelTestPort() {
    return static_cast<uint16_t>(20000 + (getpid() % 10000));
}

struct ChannelFixture : public ::testing::Test
{
	static Logger logger;
	static boost::asio::io_context serverIo;
	static boost::asio::io_context clientIo;
	static std::unique_ptr<SocketConnection> serverConn;
	static std::unique_ptr<SocketConnection> clientConn;
	static std::unique_ptr<ServerSecureChannel> server;
	static std::unique_ptr<ClientSecureChannel> client;
	static bool handshakeDone;

	static void SetUpTestSuite()
	{
		if (sodium_init() < 0) return;

		const uint16_t port = channelTestPort();

		SocketAcceptor acceptor;
		acceptor.Initialize(serverIo, port);

		std::thread clientThread(
			[port]()
			{
				SocketConnector connector;
				connector.Initialize(clientIo);
				connector.Connect("localhost", port, clientConn);
			});

		acceptor.Accept(serverConn);
		clientThread.join();

		server = std::make_unique<ServerSecureChannel>(*serverConn);
		server->setLogger(&logger);

		client = std::make_unique<ClientSecureChannel>(*clientConn);
		client->setLogger(&logger);

		bool serverResult, clientResult;

		std::thread serverThread([&]() { serverResult = server->StartTLS(); });
		clientResult = client->StartTLS();
		serverThread.join();

		handshakeDone = serverResult && clientResult;
	}

	static void TearDownTestSuite()
	{
		serverConn->Close();
		clientConn->Close();
	}
};

Logger ChannelFixture::logger(std::make_unique<ConsoleStrategy>());
boost::asio::io_context ChannelFixture::serverIo;
boost::asio::io_context ChannelFixture::clientIo;
std::unique_ptr<SocketConnection> ChannelFixture::serverConn;
std::unique_ptr<SocketConnection> ChannelFixture::clientConn;
std::unique_ptr<ServerSecureChannel> ChannelFixture::server;
std::unique_ptr<ClientSecureChannel> ChannelFixture::client;
bool ChannelFixture::handshakeDone = false;


TEST_F(ChannelFixture, HandshakeSucceeds)
{
	EXPECT_TRUE(handshakeDone);
	EXPECT_TRUE(server->isSecure());
	EXPECT_TRUE(client->isSecure());
}

TEST_F(ChannelFixture, MessageMatch)
{
	const std::string message = "hello";
	std::string received;

	std::thread serverThread([&]() { server->Send(message); });

	EXPECT_TRUE(client->Receive(received));
	serverThread.join();

	EXPECT_EQ(received, message);
}

TEST_F(ChannelFixture, StartTlsAlreadySecureReturnsFalse)
{
	ASSERT_TRUE(handshakeDone);
	EXPECT_FALSE(server->StartTLS());
	EXPECT_FALSE(client->StartTLS());
}

TEST_F(ChannelFixture, IsSecureAfterHandshake)
{
	ASSERT_TRUE(handshakeDone);
	EXPECT_TRUE(server->isSecure());
	EXPECT_TRUE(client->isSecure());
}

TEST_F(ChannelFixture, SendEmptyStringFails)
{
	ASSERT_TRUE(handshakeDone);
	EXPECT_FALSE(server->Send(""));
}

TEST_F(ChannelFixture, MultipleMessagesClientToServer)
{
	ASSERT_TRUE(handshakeDone);
	for (int i = 0; i < 3; ++i) {
		std::string msg = "msg" + std::to_string(i);
		std::string recv;
		std::thread t([&]() { client->Send(msg); });
		EXPECT_TRUE(server->Receive(recv));
		t.join();
		EXPECT_EQ(recv, msg);
	}
}