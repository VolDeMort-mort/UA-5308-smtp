#include <filesystem>
#include <gtest/gtest.h>
#include <sodium.h>
#include <unistd.h>

#include "SmtpSession.hpp"
#include "Base64Decoder.hpp"
#include "Base64Encoder.hpp"
#include "DataBaseManager.h"
#include "Repository/MessageRepository.h"
#include "Repository/UserRepository.h"
#include "SmtpResponse.hpp"
#include "schema.h"


static std::string smtpTestDbPath() {
    return "/tmp/test_smtp_session_" + std::to_string(getpid()) + ".db";
}

static std::string B64Encode(const std::string& raw)
{
	std::vector<uint8_t> bytes(raw.begin(), raw.end());
	return Base64Encoder::EncodeBase64(bytes);
}

// Build the AUTH PLAIN blob: "\0username\0password" base64 encoded
static std::string PlainCredentials(const std::string& user, const std::string& pass)
{
	std::string blob;
	blob += '\0';
	blob += user;
	blob += '\0';
	blob += pass;
	return B64Encode(blob);
}


class SmtpSessionTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		session = std::make_unique<SmtpSession>("testserver.local", nullptr, nullptr, nullptr);
	}

	void DoEhlo() { session->ProcessLine("EHLO client.test"); }

	std::unique_ptr<SmtpSession> session;
};


class SmtpSessionDbTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		if (sodium_init() < 0) FAIL() << "sodium_init() failed";

		db = std::make_unique<DataBaseManager>(smtpTestDbPath(), initSchema());
		user_repo = std::make_unique<UserRepository>(*db);
		message_repo = std::make_unique<MessageRepository>(*db);

		
		User alice;
		alice.username = "alice";
		ASSERT_TRUE(user_repo->registerUser(alice, "pass123"));

		User bob;
		bob.username = "bob";
		ASSERT_TRUE(user_repo->registerUser(bob, "letmein"));

		MakeSession();
	}

	void TearDown() override
	{
		session.reset();
		user_repo.reset();
		message_repo.reset();
		db.reset();
		std::remove(smtpTestDbPath().c_str());
	}

	void MakeSession()
	{
		session = std::make_unique<SmtpSession>("testserver.local", message_repo.get(), user_repo.get(), nullptr);
	}

	
	void AuthAsAlice()
	{
		session->SetSecure(true);
		session->ProcessLine("EHLO client.test");
		session->ProcessLine("AUTH PLAIN " + PlainCredentials("alice", "pass123"));
	}

	void SetupMailTransaction(const std::string& rcpt = "bob@testserver.local")
	{
		AuthAsAlice();
		session->ProcessLine("MAIL FROM:<alice@testserver.local>");
		session->ProcessLine("RCPT TO:<" + rcpt + ">");
	}

	std::unique_ptr<DataBaseManager> db;
	std::unique_ptr<UserRepository> user_repo;
	std::unique_ptr<MessageRepository> message_repo;
	std::unique_ptr<SmtpSession> session;
};



TEST_F(SmtpSessionTest, GreetingContainsDomain)
{
	EXPECT_NE(session->Greeting().find("testserver.local"), std::string::npos);
}

TEST_F(SmtpSessionTest, GreetingStartsWith220)
{
	EXPECT_EQ(session->Greeting().substr(0, 3), "220");
}



TEST_F(SmtpSessionTest, InitialStateIsWaitHelo)
{
	EXPECT_EQ(session->getState(), SmtpState::WAIT_HELO);
}

TEST_F(SmtpSessionTest, IsNotClosedInitially)
{
	EXPECT_FALSE(session->IsClosed());
}



TEST_F(SmtpSessionTest, HeloTransitionsToWaitMail)
{
	session->ProcessLine("HELO client.test");
	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
}

TEST_F(SmtpSessionTest, EhloTransitionsToWaitMail)
{
	session->ProcessLine("EHLO client.test");
	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
}

TEST_F(SmtpSessionTest, HeloResponseContainsDomain)
{
	std::string resp = session->ProcessLine("HELO client.test");
	EXPECT_EQ(resp.substr(0, 3), "250");
	EXPECT_NE(resp.find("testserver.local"), std::string::npos);
}

TEST_F(SmtpSessionTest, EhloResponseContainsAuthCapabilities)
{
	session->SetSecure(true);
	std::string resp = session->ProcessLine("EHLO client.test");
	EXPECT_NE(resp.find("AUTH LOGIN PLAIN"), std::string::npos);
}

TEST_F(SmtpSessionTest, EhloWithoutTlsAdversitesStarttls)
{
	std::string resp = session->ProcessLine("EHLO client.test");
	EXPECT_NE(resp.find("STARTTLS"), std::string::npos);
}

TEST_F(SmtpSessionTest, EhloWithTlsDoesNotAdvertiseStarttls)
{
	session->SetSecure(true);
	std::string resp = session->ProcessLine("EHLO client.test");
	EXPECT_EQ(resp.find("STARTTLS"), std::string::npos);
}


TEST_F(SmtpSessionTest, NoopReturnsOk)
{
	DoEhlo();
	std::string resp = session->ProcessLine("NOOP");
	EXPECT_EQ(resp, SmtpResponse::Ok());
}

TEST_F(SmtpSessionTest, RsetReturnsOkAndResetsToWaitMail)
{
	DoEhlo();
	std::string resp = session->ProcessLine("RSET");
	EXPECT_EQ(resp, SmtpResponse::Ok());
	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
}

TEST_F(SmtpSessionTest, QuitReturnsClosingAndClosesSession)
{
	std::string resp = session->ProcessLine("QUIT");
	EXPECT_EQ(resp, SmtpResponse::Closing());
	EXPECT_TRUE(session->IsClosed());
	EXPECT_EQ(session->getState(), SmtpState::CLOSED);
}


TEST_F(SmtpSessionTest, StarttlsInWaitMailReturnsReadyAndSetsState)
{
	DoEhlo();
	std::string resp = session->ProcessLine("STARTTLS");
	EXPECT_EQ(resp, SmtpResponse::StartTls());
	EXPECT_EQ(session->getState(), SmtpState::STARTTLS);
}

TEST_F(SmtpSessionTest, StarttlsBeforeEhloBadSequence)
{
	std::string resp = session->ProcessLine("STARTTLS");
	EXPECT_EQ(resp, SmtpResponse::BadSequence());
}


TEST_F(SmtpSessionTest, MailInWaitHeloReturnsBadSequence)
{
	std::string resp = session->ProcessLine("MAIL FROM:<some@gmail.com>");
	EXPECT_EQ(resp, SmtpResponse::BadSequence());
}

TEST_F(SmtpSessionTest, RcptBeforeMailReturnsBadSequence)
{
	DoEhlo();
	std::string resp = session->ProcessLine("RCPT TO:<some@gmail.com>");
	EXPECT_EQ(resp, SmtpResponse::BadSequence());
}

TEST_F(SmtpSessionTest, DataBeforeRcptReturnsBadSequence)
{
	DoEhlo();
	std::string resp = session->ProcessLine("DATA");
	EXPECT_EQ(resp, SmtpResponse::BadSequence());
}


TEST_F(SmtpSessionTest, LineLongerThan512BytesReturnsSyntaxError)
{
	std::string longLine(513, 'A');
	std::string resp = session->ProcessLine(longLine);
	EXPECT_EQ(resp, SmtpResponse::SyntaxError());
}

TEST_F(SmtpSessionTest, LineExactly512BytesIsAccepted)
{
	std::string line(512, 'A');
	std::string resp = session->ProcessLine(line);
	EXPECT_NE(resp, SmtpResponse::SyntaxError());
}

TEST_F(SmtpSessionTest, UnknownCommandReturnsNotImplemented)
{
	DoEhlo();
	std::string resp = session->ProcessLine("VRFY user@example.com");
	EXPECT_EQ(resp, SmtpResponse::NotImplemented());
}

TEST_F(SmtpSessionTest, ResetToHeloSetsWaitHeloState)
{
	DoEhlo();
	session->ResetToHelo();
	EXPECT_EQ(session->getState(), SmtpState::WAIT_HELO);
}


TEST_F(SmtpSessionDbTest, AuthBeforeEhloBadSequence)
{
	session->SetSecure(true);
	std::string resp = session->ProcessLine("AUTH PLAIN " + PlainCredentials("alice", "pass123"));
	EXPECT_EQ(resp, SmtpResponse::BadSequence());
}

TEST_F(SmtpSessionDbTest, AuthWithoutTlsReturnsEncryptionRequired)
{
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("AUTH PLAIN " + PlainCredentials("alice", "pass123"));
	EXPECT_EQ(resp, SmtpResponse::EncryptionRequired());
}

TEST_F(SmtpSessionDbTest, AuthPlainInlineSuccess)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("AUTH PLAIN " + PlainCredentials("alice", "pass123"));
	EXPECT_EQ(resp, SmtpResponse::AuthSucceeded());
}

TEST_F(SmtpSessionDbTest, AuthPlainInlineWrongPasswordFails)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("AUTH PLAIN " + PlainCredentials("alice", "wrongpassword"));
	EXPECT_EQ(resp, SmtpResponse::AuthFailed());
}

TEST_F(SmtpSessionDbTest, AuthPlainInlineNonExistentUserFails)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("AUTH PLAIN " + PlainCredentials("ghost", "pass123"));
	EXPECT_EQ(resp, SmtpResponse::AuthFailed());
}

TEST_F(SmtpSessionDbTest, AuthPlainTwoStep)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");

	std::string challenge = session->ProcessLine("AUTH PLAIN");
	EXPECT_EQ(challenge.substr(0, 3), "334");
	EXPECT_EQ(session->getState(), SmtpState::AUTH_WAIT_PASS);

	std::string resp = session->ProcessLine(PlainCredentials("alice", "pass123"));
	EXPECT_EQ(resp, SmtpResponse::AuthSucceeded());
}

TEST_F(SmtpSessionDbTest, AuthLoginTwoStep)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");

	std::string challenge1 = session->ProcessLine("AUTH LOGIN");
	EXPECT_EQ(challenge1.substr(0, 3), "334");
	EXPECT_EQ(session->getState(), SmtpState::AUTH_WAIT_USER);

	std::string challenge2 = session->ProcessLine(B64Encode("alice"));
	EXPECT_EQ(challenge2.substr(0, 3), "334");
	EXPECT_EQ(session->getState(), SmtpState::AUTH_WAIT_PASS);

	std::string resp = session->ProcessLine(B64Encode("pass123"));
	EXPECT_EQ(resp, SmtpResponse::AuthSucceeded());
}

TEST_F(SmtpSessionDbTest, AuthLoginWrongPasswordFails)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	session->ProcessLine("AUTH LOGIN");
	session->ProcessLine(B64Encode("alice"));
	std::string resp = session->ProcessLine(B64Encode("wrongpassword"));
	EXPECT_EQ(resp, SmtpResponse::AuthFailed());
	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
}

TEST_F(SmtpSessionDbTest, AuthUnknownMechanismReturnsError)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("AUTH GSSAPI sometoken");
	EXPECT_EQ(resp, SmtpResponse::UnrecognizedAuthMech());
}

TEST_F(SmtpSessionDbTest, DoubleAuthReturnsBadSequence)
{
	AuthAsAlice();
	std::string resp = session->ProcessLine("AUTH PLAIN " + PlainCredentials("alice", "pass123"));
	EXPECT_EQ(resp, SmtpResponse::BadSequence());
}

TEST_F(SmtpSessionDbTest, EhloResetsAuthenticationFlag)
{
	AuthAsAlice();
	std::string resp = session->ProcessLine("EHLO again.test");
	EXPECT_EQ(resp.substr(0, 3), "250");
	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
}


TEST_F(SmtpSessionDbTest, MailWithoutAuthReturnsAuthRequired)
{
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	EXPECT_EQ(resp, SmtpResponse::AuthRequired());
}

TEST_F(SmtpSessionDbTest, MailAfterAuthReturnsOkAndAdvancesState)
{
	AuthAsAlice();
	std::string resp = session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	EXPECT_EQ(resp, SmtpResponse::Ok());
	EXPECT_EQ(session->getState(), SmtpState::WAIT_RCPT);
}

TEST_F(SmtpSessionDbTest, RcptAfterMailReturnsOk)
{
	AuthAsAlice();
	session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	std::string resp = session->ProcessLine("RCPT TO:<bob@testserver.local>");
	EXPECT_EQ(resp, SmtpResponse::Ok());
	EXPECT_EQ(session->getState(), SmtpState::WAIT_RCPT);
}

TEST_F(SmtpSessionDbTest, MultipleRcptAllowed)
{
	AuthAsAlice();
	session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	session->ProcessLine("RCPT TO:<bob@testserver.local>");
	std::string resp = session->ProcessLine("RCPT TO:<alice@testserver.local>");
	EXPECT_EQ(resp, SmtpResponse::Ok());
}

TEST_F(SmtpSessionDbTest, DataAfterRcptReturnsStartMailInput)
{
	SetupMailTransaction();
	std::string resp = session->ProcessLine("DATA");
	EXPECT_EQ(resp, SmtpResponse::StartMailInput());
	EXPECT_EQ(session->getState(), SmtpState::RECEIVING_DATA);
}

TEST_F(SmtpSessionDbTest, BodyLinesReturnEmptyString)
{
	SetupMailTransaction();
	session->ProcessLine("DATA");
	std::string resp = session->ProcessLine("Subject: Hello");
	EXPECT_TRUE(resp.empty());
}

TEST_F(SmtpSessionDbTest, EndOfDataResetsStateToWaitMail)
{
	std::filesystem::create_directories("mailstore/bob");

	SetupMailTransaction("bob@testserver.local");
	session->ProcessLine("DATA");
	session->ProcessLine("From: alice@testserver.local");
	session->ProcessLine("To: bob@testserver.local");
	session->ProcessLine("Subject: Test");
	session->ProcessLine("");
	session->ProcessLine("Hello Bob.");

	std::string resp = session->ProcessLine(".");

	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
	
	EXPECT_TRUE(resp == SmtpResponse::Ok() || resp == SmtpResponse::MessageStorageFailed());
}

TEST_F(SmtpSessionDbTest, MessageTooLargeReturnsErrorAndResetsToWaitMail)
{
	SetupMailTransaction();
	session->ProcessLine("DATA");

	// Flood the session with data past the 10 MB limit.
	constexpr size_t CHUNK = 512;
	const std::string chunk(CHUNK, 'X');
	constexpr size_t MAX_MSG = 10 * 1024 * 1024;

	std::string resp;
	for (size_t sent = 0; sent < MAX_MSG + CHUNK; sent += CHUNK)
	{
		resp = session->ProcessLine(chunk);
		if (!resp.empty()) break;
	}

	EXPECT_EQ(resp, SmtpResponse::MessageTooLarge());
	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
}

TEST_F(SmtpSessionDbTest, RsetDuringMailTransactionResetsState)
{
	AuthAsAlice();
	session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	session->ProcessLine("RCPT TO:<bob@testserver.local>");

	std::string resp = session->ProcessLine("RSET");
	EXPECT_EQ(resp, SmtpResponse::Ok());
	EXPECT_EQ(session->getState(), SmtpState::WAIT_MAIL);
}

TEST_F(SmtpSessionDbTest, SecondMailTransactionAfterRsetSucceeds)
{
	AuthAsAlice();
	session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	session->ProcessLine("RSET");

	std::string resp = session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	EXPECT_EQ(resp, SmtpResponse::Ok());
}


TEST_F(SmtpSessionDbTest, SetSecureFalseEhloShowsStarttls)
{
	// Default: not secure.
	std::string resp = session->ProcessLine("EHLO client.test");
	EXPECT_NE(resp.find("STARTTLS"), std::string::npos);
}

TEST_F(SmtpSessionDbTest, SetSecureTrueEhloHidesStarttls)
{
	session->SetSecure(true);
	std::string resp = session->ProcessLine("EHLO client.test");
	EXPECT_EQ(resp.find("STARTTLS"), std::string::npos);
}

TEST_F(SmtpSessionDbTest, AuthLoginInvalidBase64Fails)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	session->ProcessLine("AUTH LOGIN");
	std::string resp = session->ProcessLine("!!!not-base64!!!");
	EXPECT_NE(resp.find("535"), std::string::npos);
}

TEST_F(SmtpSessionDbTest, AuthPlainInvalidBase64Fails)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("AUTH PLAIN !!!not-base64!!!");
	EXPECT_NE(resp.find("535"), std::string::npos);
}

TEST_F(SmtpSessionDbTest, AuthPlainMissingNullSeparatorFails)
{
	session->SetSecure(true);
	session->ProcessLine("EHLO client.test");
	std::string resp = session->ProcessLine("AUTH PLAIN " + B64Encode("nousepattern"));
	EXPECT_NE(resp.find("535"), std::string::npos);
}

TEST_F(SmtpSessionDbTest, SaveMessagePlainText)
{
	SetupMailTransaction("alice@testserver.local");
	session->ProcessLine("DATA");

	std::string body =
		"From: alice@testserver.local\r\n"
		"To: alice@testserver.local\r\n"
		"Subject: plain\r\n"
		"Content-Type: text/plain\r\n"
		"\r\n"
		"Hello plain text\r\n";

	for (const auto& line : {
		"From: alice@testserver.local",
		"To: alice@testserver.local",
		"Subject: plain",
		"Content-Type: text/plain",
		"",
		"Hello plain text"
	}) {
		session->ProcessLine(line);
	}

	std::string resp = session->ProcessLine(".");
	EXPECT_TRUE(resp == SmtpResponse::Ok() || resp == SmtpResponse::MessageStorageFailed());
}

TEST_F(SmtpSessionDbTest, SaveMessageWithHtml)
{
	SetupMailTransaction("alice@testserver.local");
	session->ProcessLine("DATA");

	for (const auto& line : {
		"From: alice@testserver.local",
		"To: alice@testserver.local",
		"Subject: html mail",
		"MIME-Version: 1.0",
		"Content-Type: multipart/alternative; boundary=\"bound1\"",
		"",
		"--bound1",
		"Content-Type: text/plain",
		"",
		"Plain part",
		"--bound1",
		"Content-Type: text/html",
		"",
		"<b>HTML part</b>",
		"--bound1--"
	}) {
		session->ProcessLine(line);
	}

	std::string resp = session->ProcessLine(".");
	EXPECT_TRUE(resp == SmtpResponse::Ok() || resp == SmtpResponse::MessageStorageFailed());
}

TEST_F(SmtpSessionDbTest, SaveMessageRecipientNotInDb)
{
	AuthAsAlice();
	session->ProcessLine("MAIL FROM:<alice@testserver.local>");
	session->ProcessLine("RCPT TO:<newuser@testserver.local>");
	session->ProcessLine("DATA");
	session->ProcessLine("Subject: hello");
	session->ProcessLine("");
	session->ProcessLine("body");
	std::string resp = session->ProcessLine(".");
	EXPECT_TRUE(resp == SmtpResponse::Ok() || resp == SmtpResponse::MessageStorageFailed());
}