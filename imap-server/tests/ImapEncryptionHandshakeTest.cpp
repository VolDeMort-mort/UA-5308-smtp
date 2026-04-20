#include <boost/asio.hpp>
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>

#include "ClientSecureChannel.hpp"
#include "DataBaseManager.h"
#include "ILoggerStrategy.h"
#include "ImapConfig.hpp"
#include "ImapServer.hpp"
#include "Logger.h"
#include "ServerSecureChannel.hpp"
#include "SocketConnection.hpp"
#include "SocketConnector.hpp"
#include "ThreadPool.h"
#include "schema.h"

class NullStrategy : public ILoggerStrategy
{
public:
	virtual std::string SpecificLog(LogLevel lvl, const std::string& msg) { return {}; }
	virtual bool IsValid() { return true; }
	virtual bool Write(const std::string& message) { return true; }
	virtual void Flush() {};
};

static std::string tempDbPath()
{
	return (std::filesystem::temp_directory_path() / "test_imap_tls.db").string();
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
		config.PORT = 30002;
		config.TIMEOUT_MINS = 5;
		config.WORKER_THREADS = 2;

		std::string m_dbPath = tempDbPath();
		db = std::make_unique<DataBaseManager>(m_dbPath, initSchema());

		pool = std::make_unique<ThreadPool>();
		pool->initialize(config.WORKER_THREADS);
		pool->set_logger(&logger);

		imapServer = std::make_unique<ImapServer>(serverIo, logger, *db, *pool, config);
		serverThread = std::thread([]() { imapServer->Start(); });
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	static void TearDownTestSuite()
	{
		serverIo.stop();
		if (serverThread.joinable())
		{
			serverThread.join();
		}
		pool->terminate();
		db.reset();

		std::filesystem::remove(tempDbPath()); 
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
		if (clientConn && clientConn->IsOpen())
		{
			clientConn->Close();
		}
	}

	void sendPlain(const std::string& line) 
	{ 
		clientConn->Send(line + "\r\n"); 
	}

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
		if (resp.find(tag + " OK") == std::string::npos)
		{
			return false;
		}

		tlsClient = std::make_unique<ClientSecureChannel>(*clientConn);
		tlsClient->setLogger(&logger);

		return tlsClient->StartTLS();
	}
};

Logger ImapStartTlsFixture::logger(std::make_unique<NullStrategy>());
boost::asio::io_context ImapStartTlsFixture::serverIo;
boost::asio::io_context ImapStartTlsFixture::clientIo;
std::unique_ptr<DataBaseManager> ImapStartTlsFixture::db;
std::unique_ptr<ThreadPool> ImapStartTlsFixture::pool;
ImapConfig ImapStartTlsFixture::config;
std::unique_ptr<ImapServer> ImapStartTlsFixture::imapServer;
std::thread ImapStartTlsFixture::serverThread;

TEST_F(ImapStartTlsFixture, ClientDisconnectBeforeTlsIsHandledGracefully)
{
	clientConn->Close();
	std::this_thread::sleep_for(std::chrono::milliseconds(50));
	SocketConnector connector;
	connector.Initialize(clientIo);
	std::unique_ptr<SocketConnection> newConn;
	EXPECT_NO_THROW(connector.Connect("localhost", config.PORT, newConn));
}

TEST_F(ImapStartTlsFixture, UnknownCommandBeforeTlsReturnsBad)
{
	sendPlain("X001 GIBBERISH");
	std::string resp = recvPlain();
	EXPECT_NE(resp.find("BAD"), std::string::npos) << "Got: " << resp;
}

TEST_F(ImapStartTlsFixture, TlsHandshakeSucceeds)
{
	EXPECT_TRUE(upgradeToTls("H001")) << "TLS handshake should succeed after STARTTLS OK";
	EXPECT_TRUE(tlsClient->isSecure());
}

TEST_F(ImapStartTlsFixture, UnknownCommandAfterTlsReturnsBad)
{
	ASSERT_TRUE(upgradeToTls());

	std::string resp;
	tlsClient->Send("U001 NONSENSE\r\n");
	EXPECT_TRUE(tlsClient->Receive(resp));
	EXPECT_NE(resp.find("BAD"), std::string::npos) << "Got: " << resp;
}

TEST_F(ImapStartTlsFixture, StartTlsWithArgumentsReturnsBad)
{
	sendPlain("S001 STARTTLS unexpected-arg");
	std::string resp = recvPlain();
	EXPECT_NE(resp.find("BAD"), std::string::npos) << "Got: " << resp;
}

TEST_F(ImapStartTlsFixture, EmptyCommandDoesNotCrash)
{
	sendPlain("");
	std::string resp = recvPlain();
	EXPECT_NE(resp.find("BAD"), std::string::npos);
}

TEST_F(ImapStartTlsFixture, OversizedCommandIsRejected)
{
	std::string huge = "A001 " + std::string(100000, 'X');
	sendPlain(huge);
	std::string resp = recvPlain();
	EXPECT_NE(resp.find("BAD"), std::string::npos) << "Got: " << resp;
}

TEST_F(ImapStartTlsFixture, SecondStartTlsRejectedWithBad)
{
	ASSERT_TRUE(upgradeToTls("T001"));

	std::string received;
	tlsClient->Send("T002 STARTTLS\r\n");
	EXPECT_TRUE(tlsClient->Receive(received));

	EXPECT_NE(received.find("BAD"), std::string::npos) << "Second STARTTLS must return BAD, got: " << received;
}

TEST_F(ImapStartTlsFixture, NoopOverTlsReturnsOk)
{
	ASSERT_TRUE(upgradeToTls());

	std::string received;
	tlsClient->Send("N001 NOOP\r\n");
	EXPECT_TRUE(tlsClient->Receive(received));

	EXPECT_NE(received.find("N001 OK"), std::string::npos) << "NOOP over TLS should return OK, got: " << received;
}

TEST_F(ImapStartTlsFixture, MessageIntegrityOverTls)
{
	ASSERT_TRUE(upgradeToTls());

	for (int i = 1; i <= 5; ++i)
	{
		std::string tag = "M00" + std::to_string(i);
		std::string recv;

		tlsClient->Send(tag + " NOOP\r\n");
		EXPECT_TRUE(tlsClient->Receive(recv));

		EXPECT_NE(recv.find(tag + " OK"), std::string::npos) << "NOOP #" << i << " did not return OK, got: " << recv;
	}
}