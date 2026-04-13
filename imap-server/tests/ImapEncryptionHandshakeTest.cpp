#include <boost/asio.hpp>
#include <chrono>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <unistd.h>
#include <thread>

#include <sodium.h>

#include "ClientSecureChannel.hpp"
#include "ConsoleStrategy.h"
#include "DataBaseManager.h"
#include "ImapConfig.hpp"
#include "ImapServer.hpp"
#include "Logger.h"
#include "ServerSecureChannel.hpp"
#include "SocketAcceptor.hpp"
#include "SocketConnection.hpp"
#include "SocketConnector.hpp"
#include "ThreadPool.h"
#include "UserDAL.h"
#include "schema.h"

static uint16_t imapTlsTestPort() {
    return static_cast<uint16_t>(22000 + (getpid() % 10000));
}

struct ImapStartTlsFixture : public ::testing::Test
{
	static Logger logger;
	static boost::asio::io_context serverIo;
	static boost::asio::io_context clientIo;
	static std::unique_ptr<DataBaseManager> db;
	static std::unique_ptr<ThreadPool> pool;
	static ImapConfig config;
	static std::unique_ptr<ImapServer> imapServer;
	static std::thread serverThread;

	std::unique_ptr<SocketConnection> clientConn;
	std::unique_ptr<ClientSecureChannel> tlsClient;

	static void SetUpTestSuite()
	{
		if (sodium_init() < 0) return;

		config.PORT = imapTlsTestPort();
		config.TIMEOUT_MINS = 5;
		config.WORKER_THREADS = 2;

		db = std::make_unique<DataBaseManager>(":memory:", initSchema(),
											   std::shared_ptr<ILogger>(&logger, [](ILogger*) {}));

		pool = std::make_unique<ThreadPool>();
		pool->initialize(config.WORKER_THREADS);
		pool->set_logger(&logger);

		imapServer = std::make_unique<ImapServer>(serverIo, logger, *db, *pool, config);

		serverThread = std::thread([]() { imapServer->Start(); });

		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	static void TearDownTestSuite()
	{
		serverIo.stop();
		if (serverThread.joinable()) serverThread.join();
	}

	void SetUp() override
	{
		SocketConnector connector;
		connector.Initialize(clientIo);
		connector.Connect("localhost", config.PORT, clientConn);

		std::string banner;
		ASSERT_TRUE(clientConn->Receive(banner));
		ASSERT_NE(banner.find("* OK"), std::string::npos) << "Unexpected banner: " << banner;
	}

	void TearDown() override
	{
		tlsClient.reset();
		if (clientConn) clientConn->Close();
	}

	void sendPlain(const std::string& line) { clientConn->Send(line + "\r\n"); }

	std::string recvPlain()
	{
		std::string line;
		clientConn->Receive(line);
		return line;
	}

	bool upgradeToTls(const std::string& tag = "T001")
	{
		sendPlain(tag + " STARTTLS");
		std::string resp = recvPlain();
		if (resp.find(tag + " OK") == std::string::npos) return false;

		tlsClient = std::make_unique<ClientSecureChannel>(*clientConn);
		tlsClient->setLogger(&logger);

		return tlsClient->StartTLS();
	}
};

Logger ImapStartTlsFixture::logger(std::make_unique<ConsoleStrategy>());
boost::asio::io_context ImapStartTlsFixture::serverIo;
boost::asio::io_context ImapStartTlsFixture::clientIo;
std::unique_ptr<DataBaseManager> ImapStartTlsFixture::db;
std::unique_ptr<ThreadPool> ImapStartTlsFixture::pool;
ImapConfig ImapStartTlsFixture::config;
std::unique_ptr<ImapServer> ImapStartTlsFixture::imapServer;
std::thread ImapStartTlsFixture::serverThread;

TEST_F(ImapStartTlsFixture, TlsHandshakeSucceeds)
{
	EXPECT_TRUE(upgradeToTls("H001")) << "TLS handshake should succeed after STARTTLS OK";
	EXPECT_TRUE(tlsClient->isSecure());
}

TEST_F(ImapStartTlsFixture, SecondStartTlsRejectedWithBad)
{
	ASSERT_TRUE(upgradeToTls("T001"));

	std::string received;
	std::thread t([&]() { tlsClient->Send("T002 STARTTLS\r\n"); });
	EXPECT_TRUE(tlsClient->Receive(received));
	t.join();

	EXPECT_NE(received.find("BAD"), std::string::npos) << "Second STARTTLS must return BAD, got: " << received;
}

TEST_F(ImapStartTlsFixture, NoopOverTlsReturnsOk)
{
	ASSERT_TRUE(upgradeToTls());

	std::string received;
	std::thread t([&]() { tlsClient->Send("N001 NOOP\r\n"); });
	EXPECT_TRUE(tlsClient->Receive(received));
	t.join();

	EXPECT_NE(received.find("N001 OK"), std::string::npos) << "NOOP over TLS should return OK, got: " << received;
}

TEST_F(ImapStartTlsFixture, MessageIntegrityOverTls)
{
	ASSERT_TRUE(upgradeToTls());

	for (int i = 1; i <= 5; ++i)
	{
		std::string tag = "M00" + std::to_string(i);
		std::string send = tag + " NOOP\r\n";
		std::string recv;

		std::thread t([&]() { tlsClient->Send(send); });
		EXPECT_TRUE(tlsClient->Receive(recv));
		t.join();

		EXPECT_NE(recv.find(tag + " OK"), std::string::npos) << "NOOP #" << i << " did not return OK, got: " << recv;
	}
}
