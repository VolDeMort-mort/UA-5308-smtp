#include "SocketAcceptor.hpp"
#include "SocketConnector.hpp"
#include "SocketConnection.hpp"
#include "ServerSecureChannel.hpp"
#include "ClientSecureChannel.hpp"
#include "Logger.h"
#include "FileStrategy.h"

#include <gtest/gtest.h>

using boost::asio::ip::tcp;

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
		constexpr uint16_t port = 29999;

		SocketAcceptor acceptor;
		acceptor.initialize(serverIo, port);

		std::thread clientThread(
			[port]()
			{
				SocketConnector connector;
				connector.initialize(clientIo);
				connector.connect("localhost", port, clientConn);
			});

		acceptor.accept(serverConn);
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
		serverConn->close();
		clientConn->close();
	}
};

Logger ChannelFixture::logger(std::make_unique<FileStrategy>(LogLevel::TRACE));
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

	client->Receive(received);
	serverThread.join();

	EXPECT_EQ(received, message);
}
