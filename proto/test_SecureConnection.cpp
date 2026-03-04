#include <gtest/gtest.h>
#include <sodium.h>
#include <thread>

#include "SecureConnection.hpp"

class SecureConnectionTest : public ::testing::Test
{
protected:
	static void SetUpTestSuite() { ASSERT_NE(sodium_init(), -1); }

	void SetUp() override
	{
		boost::asio::ip::tcp::acceptor acceptor(m_io, {boost::asio::ip::tcp::v4(), 0});
		auto port = acceptor.local_endpoint().port();

		boost::asio::ip::tcp::socket client_sock(m_io);
		boost::asio::ip::tcp::socket server_sock(m_io);

		std::thread accept_thread([&]() { acceptor.accept(server_sock); });
		client_sock.connect({boost::asio::ip::address_v4::loopback(), port});
		accept_thread.join();

		m_client = std::make_unique<SecureConnection>(std::move(client_sock));
		m_server = std::make_unique<SecureConnection>(std::move(server_sock));

		EphemeralKeys client_keys{};
		EphemeralKeys server_keys{};
		crypto_kx_keypair(client_keys.pk.data(), client_keys.sk.data());
		crypto_kx_keypair(server_keys.pk.data(), server_keys.sk.data());

		bool server_ok = false;
		std::thread server_thread([&]()
								  { server_ok = m_server->handshake(crypto_kx_server_session_keys, server_keys); });

		bool client_ok = m_client->handshake(crypto_kx_client_session_keys, client_keys);
		server_thread.join();

		ASSERT_TRUE(client_ok);
		ASSERT_TRUE(server_ok);
	}

	boost::asio::io_context m_io;
	std::unique_ptr<SecureConnection> m_client;
	std::unique_ptr<SecureConnection> m_server;
};

TEST_F(SecureConnectionTest, HandshakeProducesMatchingKeys)
{
	EXPECT_EQ(m_client->m_tx_key, m_server->m_rx_key);
	EXPECT_EQ(m_client->m_rx_key, m_server->m_tx_key);
}

TEST_F(SecureConnectionTest, SendReceiveRoundtrip)
{
	const std::string message = "MAIL FROM:<alice@example.com>\r\n";

	ASSERT_TRUE(m_client->send(message));

	std::string received;
	ASSERT_TRUE(m_server->receive(received));

	EXPECT_EQ(received, message);
}
