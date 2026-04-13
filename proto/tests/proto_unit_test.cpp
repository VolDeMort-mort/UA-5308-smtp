#include <gtest/gtest.h>

#include <thread>
#include <unistd.h>

#include "SmtpResponse.hpp"
#include "ImapConnection.hpp"
#include "SocketAcceptor.hpp"
#include "SocketConnector.hpp"
#include "SocketConnection.hpp"

static uint16_t imapConnTestPort() {
    return static_cast<uint16_t>(21000 + (getpid() % 10000));
}

// ═════════════════════════════════════════════════════════════════════════════
// SmtpResponse tests
// ═════════════════════════════════════════════════════════════════════════════

TEST(SmtpResponseTest, ServiceReady) {
    EXPECT_EQ(SmtpResponse::ServiceReady("mail.example.com"),
              "220 mail.example.com Service ready\r\n");
}

TEST(SmtpResponseTest, Ok) {
    EXPECT_EQ(SmtpResponse::Ok(), "250 OK\r\n");
}

TEST(SmtpResponseTest, Hello) {
    EXPECT_EQ(SmtpResponse::Hello("smtp.test.com"),
              "250 smtp.test.com greets you\r\n");
}

TEST(SmtpResponseTest, EhloWithTlsActive) {
    std::string r = SmtpResponse::Ehlo("smtp.test.com", true);
    EXPECT_NE(r.find("250-smtp.test.com"), std::string::npos);
    EXPECT_EQ(r.find("STARTTLS"), std::string::npos);
    EXPECT_NE(r.find("AUTH LOGIN PLAIN"), std::string::npos);
}

TEST(SmtpResponseTest, EhloWithoutTls) {
    std::string r = SmtpResponse::Ehlo("smtp.test.com", false);
    EXPECT_NE(r.find("STARTTLS"), std::string::npos);
    EXPECT_NE(r.find("AUTH LOGIN PLAIN"), std::string::npos);
}

TEST(SmtpResponseTest, StartMailInput) {
    EXPECT_EQ(SmtpResponse::StartMailInput(), "354 End data with <CR><LF>.<CR><LF>\r\n");
}

TEST(SmtpResponseTest, MessageStorageFailed) {
    EXPECT_EQ(SmtpResponse::MessageStorageFailed(), "452 Message storage failed\r\n");
}

TEST(SmtpResponseTest, BadSequence) {
    EXPECT_EQ(SmtpResponse::BadSequence(), "503 Bad sequence of commands\r\n");
}

TEST(SmtpResponseTest, MessageTooLarge) {
    EXPECT_EQ(SmtpResponse::MessageTooLarge(), "552 Message size exceeds fixed maximum\r\n");
}

TEST(SmtpResponseTest, SyntaxError) {
    EXPECT_EQ(SmtpResponse::SyntaxError(), "500 Syntax error\r\n");
}

TEST(SmtpResponseTest, NotImplemented) {
    EXPECT_EQ(SmtpResponse::NotImplemented(), "502 Command not implemented\r\n");
}

TEST(SmtpResponseTest, Closing) {
    EXPECT_EQ(SmtpResponse::Closing(), "221 Bye\r\n");
}

TEST(SmtpResponseTest, StartTls) {
    EXPECT_EQ(SmtpResponse::StartTls(), "220 Ready to start TLS\r\n");
}

TEST(SmtpResponseTest, ReadyToStartTLS) {
    EXPECT_EQ(SmtpResponse::ReadyToStartTLS("smtp.test.com"),
              "250-smtp.test.com STARTTLS\r\n");
}

TEST(SmtpResponseTest, AuthChallenge) {
    EXPECT_EQ(SmtpResponse::AuthChallenge("dXNlcm5hbWU="),
              "334 dXNlcm5hbWU=\r\n");
}

TEST(SmtpResponseTest, AuthSucceeded) {
    EXPECT_EQ(SmtpResponse::AuthSucceeded(), "235 Authentication successful\r\n");
}

TEST(SmtpResponseTest, AuthFailed) {
    EXPECT_EQ(SmtpResponse::AuthFailed(), "535 Authentication credentials invalid\r\n");
}

TEST(SmtpResponseTest, AuthRequired) {
    EXPECT_EQ(SmtpResponse::AuthRequired(), "530 Authentication required\r\n");
}

TEST(SmtpResponseTest, EncryptionRequired) {
    EXPECT_EQ(SmtpResponse::EncryptionRequired(),
              "538 Encryption required for requested auth mechanism\r\n");
}

TEST(SmtpResponseTest, UnrecognizedAuthMech) {
    EXPECT_EQ(SmtpResponse::UnrecognizedAuthMech(),
              "504 Unrecognized authentication mechanism\r\n");
}

// ═════════════════════════════════════════════════════════════════════════════
// ImapConnection tests (live socket pair)
// ═════════════════════════════════════════════════════════════════════════════

struct ImapConnFixture : public ::testing::Test {
    static boost::asio::io_context serverIo;
    static boost::asio::io_context clientIo;
    static std::unique_ptr<SocketConnection> serverConn;
    static std::unique_ptr<SocketConnection> clientConn;
    static bool pairReady;

    static void SetUpTestSuite() {
        const uint16_t port = imapConnTestPort();

        SocketAcceptor acceptor;
        acceptor.Initialize(serverIo, port);

        std::thread clientThread([port]() {
            SocketConnector connector;
            connector.Initialize(clientIo);
            connector.Connect("localhost", port, clientConn);
        });

        acceptor.Accept(serverConn);
        clientThread.join();

        pairReady = (serverConn != nullptr) && (clientConn != nullptr);
    }

    static void TearDownTestSuite() {
        if (serverConn) serverConn->Close();
        if (clientConn) clientConn->Close();
    }
};

boost::asio::io_context ImapConnFixture::serverIo;
boost::asio::io_context ImapConnFixture::clientIo;
std::unique_ptr<SocketConnection> ImapConnFixture::serverConn;
std::unique_ptr<SocketConnection> ImapConnFixture::clientConn;
bool ImapConnFixture::pairReady = false;

TEST_F(ImapConnFixture, IsOpenReturnsTrueOnLiveSocket) {
    ASSERT_TRUE(pairReady);
    EXPECT_TRUE(clientConn->IsOpen());
    EXPECT_TRUE(serverConn->IsOpen());
}

TEST_F(ImapConnFixture, SendAndReceive) {
    ASSERT_TRUE(pairReady);

    const std::string msg = "A001 LOGIN user pass\r\n";
    std::string received;

    std::thread sender([&]() { serverConn->Send(msg); });
    EXPECT_TRUE(clientConn->Receive(received));
    sender.join();

    EXPECT_EQ(received, "A001 LOGIN user pass");
}

TEST_F(ImapConnFixture, SendRawAndReceiveRaw) {
    ASSERT_TRUE(pairReady);

    const std::vector<unsigned char> data = {0x01, 0x02, 0x03, 0x04, 0x05};
    std::vector<unsigned char> buf(data.size());

    std::thread sender([&]() {
        serverConn->SendRaw(data.data(), data.size());
    });
    EXPECT_TRUE(clientConn->ReceiveRaw(buf.data(), buf.size()));
    sender.join();

    EXPECT_EQ(buf, data);
}

// ─────────────────────────────────────────────────────────────────────────────
// ImapConnection on its own socket: IsOpen / Close / Send on closed socket
// ─────────────────────────────────────────────────────────────────────────────

TEST(ImapConnectionTest, IsOpenFalseAfterClose) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    ImapConnection conn(sock);

    EXPECT_FALSE(conn.IsOpen());
}

TEST(ImapConnectionTest, CloseOnClosedSocketDoesNotCrash) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    ImapConnection conn(sock);

    EXPECT_NO_FATAL_FAILURE(conn.Close());
}

TEST(ImapConnectionTest, SendOnClosedSocketFails) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    ImapConnection conn(sock);

    EXPECT_FALSE(conn.Send("* OK IMAP ready\r\n"));
}

TEST(ImapConnectionTest, ReceiveOnClosedSocketFails) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    ImapConnection conn(sock);

    std::string out;
    EXPECT_FALSE(conn.Receive(out));
}

TEST(ImapConnectionTest, SendRawOnClosedSocketFails) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    ImapConnection conn(sock);

    const unsigned char buf[] = {0x01, 0x02};
    EXPECT_FALSE(conn.SendRaw(buf, sizeof(buf)));
}

TEST(ImapConnectionTest, ReceiveRawOnClosedSocketFails) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    ImapConnection conn(sock);

    unsigned char buf[4];
    EXPECT_FALSE(conn.ReceiveRaw(buf, sizeof(buf)));
}

// ─────────────────────────────────────────────────────────────────────────────
// SocketAcceptor: not-initialized paths + port-in-use
// ─────────────────────────────────────────────────────────────────────────────

TEST(SocketAcceptorTest, StopWhenNotInitializedReturnsFalse) {
    SocketAcceptor acc;
    EXPECT_FALSE(acc.Stop());
}

TEST(SocketAcceptorTest, AcceptWhenNotInitializedReturnsFalse) {
    SocketAcceptor acc;
    std::unique_ptr<SocketConnection> conn;
    EXPECT_FALSE(acc.Accept(conn));
}

TEST(SocketAcceptorTest, InitializeSucceeds) {
    boost::asio::io_context io;
    SocketAcceptor acc;
    const uint16_t port = static_cast<uint16_t>(23000 + (getpid() % 10000));
    EXPECT_TRUE(acc.Initialize(io, port));
    acc.Stop();
}

TEST(SocketAcceptorTest, StopAfterInitializeSucceeds) {
    boost::asio::io_context io;
    SocketAcceptor acc;
    const uint16_t port = static_cast<uint16_t>(23100 + (getpid() % 10000));
    ASSERT_TRUE(acc.Initialize(io, port));
    EXPECT_TRUE(acc.Stop());
}

TEST(SocketAcceptorTest, InitializePortAlreadyInUseReturnsFalse) {
    boost::asio::io_context io;
    SocketAcceptor acc1, acc2;
    const uint16_t port = static_cast<uint16_t>(23200 + (getpid() % 10000));
    ASSERT_TRUE(acc1.Initialize(io, port));
    EXPECT_FALSE(acc2.Initialize(io, port));
    acc1.Stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// SocketConnection: IsOpen / SetTimeout on a real connected pair
// ─────────────────────────────────────────────────────────────────────────────

TEST(SocketConnectionTest, IsOpenFalseOnDefaultSocket) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    SocketConnection conn(std::move(sock));
    EXPECT_FALSE(conn.IsOpen());
}

TEST(SocketConnectionTest, SendOnClosedReturnsFalse) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    SocketConnection conn(std::move(sock));
    EXPECT_FALSE(conn.Send("hello\r\n"));
}

TEST(SocketConnectionTest, ReceiveOnClosedReturnsFalse) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    SocketConnection conn(std::move(sock));
    std::string out;
    EXPECT_FALSE(conn.Receive(out));
}

TEST(SocketConnectionTest, CloseOnClosedReturnsFalse) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    SocketConnection conn(std::move(sock));
    EXPECT_FALSE(conn.Close());
}

TEST(SocketConnectionTest, SendRawOnClosedReturnsFalse) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    SocketConnection conn(std::move(sock));
    const unsigned char buf[] = {0x01, 0x02};
    EXPECT_FALSE(conn.SendRaw(buf, sizeof(buf)));
}

TEST(SocketConnectionTest, ReceiveRawOnClosedReturnsFalse) {
    boost::asio::io_context io;
    boost::asio::ip::tcp::socket sock(io);
    SocketConnection conn(std::move(sock));
    unsigned char buf[4];
    EXPECT_FALSE(conn.ReceiveRaw(buf, sizeof(buf)));
}
