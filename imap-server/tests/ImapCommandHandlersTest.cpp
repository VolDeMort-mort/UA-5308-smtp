#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>
#include <sodium.h>
#include <unistd.h>

#include "DAL/FolderDAL.h"
#include "DAL/UserDAL.h"
#include "DataBaseManager.h"
#include "Entity/Folder.h"
#include "Entity/Message.h"
#include "ILogger.h"
#include "ImapCommand.hpp"
#include "ImapCommandDispatcher.hpp"
#include "Repository/MessageRepository.h"
#include "Repository/UserRepository.h"
#include "schema.h"

namespace {
std::string imapTestDbPath() {
    return "/tmp/test_imap_" + std::to_string(getpid()) + ".db";
}
std::string imapTestEmlPath(const std::string& subject) {
    return "/tmp/test_msg_" + std::to_string(getpid()) + "_" + subject + ".eml";
}
} // namespace

class MockLogger : public ILogger
{
public:
	MOCK_METHOD(void, Log, (LogLevel level, const std::string&), (override));
};

class CmdHandlerTests : public ::testing::Test
{
protected:
	void SetUp() override
	{
		if (sodium_init() < 0) {
			FAIL() << "Failed to initialize libsodium";
		}

		db = std::make_unique<DataBaseManager>(imapTestDbPath(), initSchema());
		userDal = std::make_unique<UserDAL>(db->getDB(), db->pool());
		userRepo = std::make_unique<UserRepository>(*db);;
		messRepo = std::make_unique<MessageRepository>(*db);
		dispatcher = std::make_unique<ImapCommandDispatcher>(mockLogger, *userRepo, *messRepo);
		SeedDB();
	}

	void TearDown() override
	{
		char* errMsg = nullptr;
		int rc = sqlite3_exec(db->getDB(),
							  "DELETE FROM recipients; "
							  "DELETE FROM messages; "
							  "DELETE FROM folders; "
							  "DELETE FROM users;",
							  nullptr, nullptr, &errMsg);
		if (rc != SQLITE_OK)
		{
			sqlite3_free(errMsg);
		}
		dispatcher.reset();
		db.reset();
		std::remove(imapTestDbPath().c_str());
	}

	::testing::NiceMock<MockLogger> mockLogger;
	std::unique_ptr<DataBaseManager> db;
	std::unique_ptr<UserDAL> userDal;
	std::unique_ptr<UserRepository> userRepo;
	std::unique_ptr<MessageRepository> messRepo;
	std::unique_ptr<ImapCommandDispatcher> dispatcher;
	const std::string pass = "pass123";

	void LoginAndSelect(const std::string& username, const std::string& folder)
	{
		Login(username);

		ImapCommand selectCmd;
		selectCmd.m_tag = "A001";
		selectCmd.m_type = ImapCommandType::Select;
		selectCmd.m_args = {folder};
		dispatcher->Dispatch(selectCmd);
		ASSERT_EQ(dispatcher->get_State(), SessionState::Selected);
	}

	void Login(const std::string& username)
	{
		ImapCommand loginCmd;
		loginCmd.m_tag = "A000";
		loginCmd.m_type = ImapCommandType::Login;
		loginCmd.m_args = {username, pass};
		dispatcher->Dispatch(loginCmd);
		ASSERT_TRUE(dispatcher->get_AuthenticatedUserID().has_value());
	}

	void SeedDB()
	{
		User alice, bob, charlie, dave;
		alice.username = "alice";
		bob.username = "bob";
		charlie.username = "charlie";
		dave.username = "dave";

		ASSERT_TRUE(userRepo->registerUser(alice, pass)) << userRepo->getLastError();
		ASSERT_TRUE(userRepo->registerUser(bob, pass)) << userRepo->getLastError();
		ASSERT_TRUE(userRepo->registerUser(charlie, pass)) << userRepo->getLastError();
		ASSERT_TRUE(userRepo->registerUser(dave, pass)) << userRepo->getLastError();

		FolderDAL folderDal(db->getDB(), db->pool());

		auto bobSpam = Folder{std::nullopt, 2, std::nullopt, "Spam", 1, true};
		auto bobWork = Folder{std::nullopt, 2, std::nullopt, "Work", 1, true};
		auto charlieFamily = Folder{std::nullopt, 3, std::nullopt, "Family", 1, true};
		auto aliceArchive = Folder{std::nullopt, 1, std::nullopt, "Archive", 1, true};

		ASSERT_TRUE(folderDal.insert(bobSpam)) << folderDal.getLastError();
		ASSERT_TRUE(folderDal.insert(bobWork)) << folderDal.getLastError();
		ASSERT_TRUE(folderDal.insert(charlieFamily)) << folderDal.getLastError();
		ASSERT_TRUE(folderDal.insert(aliceArchive)) << folderDal.getLastError();

		auto aliceInbox = folderDal.findByName(1, "INBOX");
		auto aliceSent = folderDal.findByName(1, "Sent");
		auto bobInbox = folderDal.findByName(2, "INBOX");
		auto bobWorkFolder = folderDal.findByName(2, "Work");
		auto charlieInbox = folderDal.findByName(3, "INBOX");
		auto charlieFamilyFolder = folderDal.findByName(3, "Family");

		ASSERT_TRUE(aliceInbox.has_value()) << "Alice INBOX not found";
		ASSERT_TRUE(aliceSent.has_value()) << "Alice Sent not found";
		ASSERT_TRUE(bobInbox.has_value()) << "Bob INBOX not found";
		ASSERT_TRUE(bobWorkFolder.has_value()) << "Bob Work not found";
		ASSERT_TRUE(charlieInbox.has_value()) << "Charlie INBOX not found";
		ASSERT_TRUE(charlieFamilyFolder.has_value()) << "Charlie Family not found";

		auto addMsg = [&](int64_t userId, int64_t folderId, const std::string& from, const std::string& subject,
						  bool flagged, bool answered)
		{
			std::string filepath = imapTestEmlPath(subject);
			std::ofstream file(filepath);
			file << "From: " << from << "\r\n";
			file << "To: recipient@test.com\r\n";
			file << "Cc: cc@test.com\r\n";
			file << "Subject: " << subject << "\r\n";
			file << "Date: Thu, 19 Mar 2026 12:00:00 +0200\r\n";
			file << "Message-ID: <" << filepath << ">\r\n";
			file << "MIME-Version: 1.0\r\n";
			file << "Content-Type: text/plain; charset=\"utf-8\"\r\n";
			file << "\r\n";
			file << "This is the body of the message.\r\n";
			file << "It contains multiple lines.\r\n";
			file.close();

			Message msg;
			msg.user_id = userId;
			msg.from_address = from;
			msg.subject = subject;
			msg.raw_file_path = filepath;
			msg.size_bytes = 21;
			msg.is_seen = false;
			msg.is_flagged = flagged;
			msg.is_answered = answered;
			msg.is_deleted = false;
			msg.is_draft = false;
			msg.is_recent = true;
			msg.internal_date = "2024-01-01 12:00:00";
			messRepo->deliver(msg, folderId);
		};

		auto addMultipartMsg =
			[&](int64_t userId, int64_t folderId, const std::string& from, const std::string& subject)
		{
			std::string filepath = imapTestEmlPath(subject);
			std::ofstream file(filepath);
			file << "From: " << from << "\r\n";
			file << "To: recipient@test.com\r\n";
			file << "Subject: " << subject << "\r\n";
			file << "Date: Thu, 19 Mar 2026 12:00:00 +0200\r\n";
			file << "Message-ID: <" << filepath << ">\r\n";
			file << "MIME-Version: 1.0\r\n";
			file << "Content-Type: multipart/alternative; boundary=\"test-boundary-123\"\r\n";
			file << "\r\n";
			file << "--test-boundary-123\r\n";
			file << "Content-Type: text/plain; charset=\"utf-8\"\r\n";
			file << "\r\n";
			file << "Plain text version.\r\n";
			file << "--test-boundary-123\r\n";
			file << "Content-Type: text/html; charset=\"utf-8\"\r\n";
			file << "<html><body><p>HTML version.</p></body></html>\r\n";
			file << "--test-boundary-123--\r\n";
			file.close();

			Message msg;
			msg.user_id = userId;
			msg.from_address = from;
			msg.subject = subject;
			msg.raw_file_path = filepath;
			msg.size_bytes = 21;
			msg.is_seen = false;
			msg.is_flagged = false;
			msg.is_answered = false;
			msg.is_deleted = false;
			msg.is_draft = false;
			msg.is_recent = true;
			msg.internal_date = "2024-01-01 12:00:00";
			messRepo->deliver(msg, folderId);
		};

		addMsg(1, aliceInbox->id.value(), "alice@test.com", "Hello Bob", false, false);
		addMsg(1, aliceInbox->id.value(), "alice@test.com", "Re: Project", true, true);
		addMsg(1, aliceInbox->id.value(), "alice@test.com", "Third message", false, false);
		addMsg(1, aliceSent->id.value(), "alice@test.com", "Hi Bob", false, false);
		addMultipartMsg(1, aliceInbox->id.value(), "alice@test.com", "MultipartMsg");

		addMsg(2, bobInbox->id.value(), "bob@test.com", "Hi Alice", false, false);
		addMsg(2, bobInbox->id.value(), "bob@test.com", "Hi Charlie", true, false);
		addMsg(2, bobWorkFolder->id.value(), "bob@test.com", "Meeting notes", true, false);

		addMsg(3, charlieInbox->id.value(), "charlie@test.com", "Party invite", true, false);
		addMsg(3, charlieInbox->id.value(), "charlie@test.com", "Party invite", true, false);
		addMsg(3, charlieFamilyFolder->id.value(), "charlie@test.com", "Re: Project update", false, false);
	}
};

TEST_F(CmdHandlerTests, HandleLogin_Success)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Login;
	cmd.m_args = {"alice", "pass123"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 OK Login successful\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_State(), SessionState::Authenticated);
	EXPECT_EQ(dispatcher->get_AuthenticatedUserID().value(), 1);
	EXPECT_EQ(dispatcher->get_AuthenticatedUserName(), "alice");
}

TEST_F(CmdHandlerTests, HandleLogin_SuccessDifferentUser)
{
	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Login;
	cmd.m_args = {"bob", "pass123"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Login successful\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_State(), SessionState::Authenticated);
	EXPECT_EQ(dispatcher->get_AuthenticatedUserID().value(), 2);
	EXPECT_EQ(dispatcher->get_AuthenticatedUserName(), "bob");
}

TEST_F(CmdHandlerTests, HandleLogin_BadCredentials)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Login;
	cmd.m_args = {"alice", "wrongpassword"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 NO Login failed\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_State(), SessionState::NonAuthenticated);
}

TEST_F(CmdHandlerTests, HandleLogin_UserNotFound)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Login;
	cmd.m_args = {"nonexistent", "password"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 NO Login failed\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_State(), SessionState::NonAuthenticated);
}

TEST_F(CmdHandlerTests, HandleSelect_Success)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Select;
	cmd.m_args = {"INBOX"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* FLAGS (\\Seen \\Answered \\Flagged \\Draft \\Deleted \\Recent)\r\n"
						   "* OK [UIDVALIDITY 1]\r\n"
						   "* OK [PERMANENTFLAGS (\\Seen \\Answered \\Flagged \\Draft \\Deleted \\*)]\r\n"
						   "* OK [UNSEEN 4]\r\n"
						   "* 4 EXISTS\r\n"
						   "* OK [UIDNEXT 5]\r\n"
						   "* 4 RECENT\r\n"
						   "A002 OK [READ-WRITE] Select completed\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_State(), SessionState::Selected);
	EXPECT_EQ(dispatcher->get_MailboxState().m_name, "INBOX");
	EXPECT_EQ(dispatcher->get_MailboxState().m_exists, 4);
}

TEST_F(CmdHandlerTests, HandleSelect_SuccessDifferentFolder)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Select;
	cmd.m_args = {"Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* FLAGS (\\Seen \\Answered \\Flagged \\Draft \\Deleted \\Recent)\r\n"
						   "* OK [UIDVALIDITY 2]\r\n"
						   "* OK [PERMANENTFLAGS (\\Seen \\Answered \\Flagged \\Draft \\Deleted \\*)]\r\n"
						   "* OK [UNSEEN 1]\r\n"
						   "* 1 EXISTS\r\n"
						   "* OK [UIDNEXT 2]\r\n"
						   "* 1 RECENT\r\n"
						   "A002 OK [READ-WRITE] Select completed\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_MailboxState().m_name, "Sent");
	EXPECT_EQ(dispatcher->get_MailboxState().m_exists, 1);
}

TEST_F(CmdHandlerTests, HandleSelect_MailboxNotFound)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Select;
	cmd.m_args = {"NonExistentFolder"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD Mailbox not found\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_State(), SessionState::Authenticated);
}

TEST_F(CmdHandlerTests, HandleList_AfterLogin_AllFolders)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::List;
	cmd.m_args = {"/", "*"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* LIST (\\HasNoChildren) \"/\" \"INBOX\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("* LIST (\\HasNoChildren) \"/\" \"Sent\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("* LIST (\\HasNoChildren) \"/\" \"Drafts\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("* LIST (\\HasNoChildren) \"/\" \"Archive\"\r\n"));

	EXPECT_THAT(response, testing::HasSubstr("A001 OK List completed\r\n"));
}

TEST_F(CmdHandlerTests, HandleList_BobFolders)
{
	Login("bob");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::List;
	cmd.m_args = {"/", "*"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("INBOX"));
	EXPECT_THAT(response, testing::HasSubstr("Sent"));
	EXPECT_THAT(response, testing::HasSubstr("Drafts"));
	EXPECT_THAT(response, testing::HasSubstr("Spam"));
	EXPECT_THAT(response, testing::HasSubstr("Work"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleList_EmptyReference)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::List;
	cmd.m_args = {"", ""};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* LIST (\\Noselect) \"/\" \"\"\r\n";
	EXPECT_THAT(response, testing::HasSubstr(expected));
}

TEST_F(CmdHandlerTests, HandleLsub_Basic)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Lsub;
	cmd.m_args = {"/", "*"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* LSUB (\\HasNoChildren) \"/\" \"INBOX\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("* LSUB (\\HasNoChildren) \"/\" \"Sent\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("* LSUB (\\HasNoChildren) \"/\" \"Drafts\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("* LSUB (\\HasNoChildren) \"/\" \"Archive\"\r\n"));

	EXPECT_THAT(response, testing::HasSubstr("A001 OK Lsub completed\r\n"));
}

TEST_F(CmdHandlerTests, HandleStatus_Success_Messages)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"INBOX", "(MESSAGES)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* STATUS INBOX (MESSAGES 4)\r\nA001 OK Status completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStatus_Success_Unseen)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"INBOX", "(MESSAGES UNSEEN)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("MESSAGES 4"));
	EXPECT_THAT(response, testing::HasSubstr("UNSEEN 4"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleStatus_Success_Recent)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"INBOX", "(RECENT)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("RECENT 4"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleStatus_MailboxNotFound)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"NonExistent", "(MESSAGES)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Mailbox not found\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStatus_InvalidDataItems)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"INBOX", "(INVALID)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Invalid status data items\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStatus_MissingArgs)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Invalid arguments number\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleFetch_Success_All_SingleMessage)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "ALL"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* 1 FETCH ("));

	EXPECT_THAT(response, testing::HasSubstr("FLAGS (\\Recent)"));

	EXPECT_THAT(response, testing::HasSubstr("RFC822.SIZE 21"));

	EXPECT_THAT(response, testing::HasSubstr("\"Hello Bob\""));

	EXPECT_THAT(response, testing::HasSubstr("INTERNALDATE \""));

	EXPECT_THAT(response, testing::HasSubstr("A002 OK Fetch completed\r\n"));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_MultipleMessages)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1:3", "ALL"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("1 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("2 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("3 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_Fast)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "FAST"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("1 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("FLAGS"));
	EXPECT_THAT(response, testing::HasSubstr("RFC822.SIZE"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
	EXPECT_THAT(response, testing::Not(testing::HasSubstr("ENVELOPE")));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_Full)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "FULL"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("1 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("FLAGS"));
	EXPECT_THAT(response, testing::HasSubstr("ENVELOPE"));
	EXPECT_THAT(response, testing::HasSubstr("BODY"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_PartialSequence)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"2", "ALL"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("2 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_InvalidSequence_OutOfRange)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"999", "ALL"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Fetch completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleFetch_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "ALL"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD No mailbox selected\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleFetch_MissingArgs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD Missing arguments\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStore_AddFlags_Seen)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"3", "+FLAGS", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* 3 FETCH (FLAGS (\\Seen \\Recent))\r\nA002 OK Store completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStore_AddFlags_Multiple)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"1", "+FLAGS", "(\\Seen \\Flagged)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* 1 FETCH (FLAGS (\\Seen \\Flagged \\Recent))\r\nA002 OK Store completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStore_RemoveFlags)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"1", "-FLAGS", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* 1 FETCH (FLAGS (\\Recent))\r\nA002 OK Store completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStore_Silent_NoResponse)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"1", "+FLAGS.SILENT", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Store completed\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_THAT(response, testing::Not(testing::HasSubstr("FETCH")));
}

TEST_F(CmdHandlerTests, HandleStore_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"1", "+FLAGS", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD No mailbox selected\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStore_MissingArgs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"1"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD Invalid arguments number\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCreate_Success)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Create;
	cmd.m_args = {"NewFolder"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 OK Create completed\r\n";
	EXPECT_EQ(response, expected);

	ImapCommand listCmd;
	listCmd.m_tag = "A002";
	listCmd.m_type = ImapCommandType::List;
	listCmd.m_args = {"/", "*"};
	std::string listResponse = dispatcher->Dispatch(listCmd);
	EXPECT_THAT(listResponse, testing::HasSubstr("NewFolder"));
}

TEST_F(CmdHandlerTests, HandleCreate_AlreadyExists)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Create;
	cmd.m_args = {"INBOX"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 NO Create failed";
	std::string actual = response.substr(0, expected.size());
	EXPECT_EQ(actual, expected);
}

TEST_F(CmdHandlerTests, HandleCreate_MissingArgs)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Create;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Invalid parameters number\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleDelete_Success)
{
	Login("alice");

	ImapCommand createCmd;
	createCmd.m_tag = "A001";
	createCmd.m_type = ImapCommandType::Create;
	createCmd.m_args = {"ToDelete"};
	dispatcher->Dispatch(createCmd);

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Delete;
	cmd.m_args = {"ToDelete"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Delete completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleDelete_FolderNotFound)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Delete;
	cmd.m_args = {"NonExistent"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD No such folder\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleDelete_SystemFolder)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Delete;
	cmd.m_args = {"INBOX"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 NO Can`t delete INBOX folder\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleDelete_MissingArgs)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Delete;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Invalid parameters number\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleRename_Success)
{
	Login("alice");

	ImapCommand createCmd;
	createCmd.m_tag = "A001";
	createCmd.m_type = ImapCommandType::Create;
	createCmd.m_args = {"OldName"};
	dispatcher->Dispatch(createCmd);

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Rename;
	cmd.m_args = {"OldName", "NewName"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Rename completed\r\n";
	EXPECT_EQ(response, expected);

	ImapCommand listCmd;
	listCmd.m_tag = "A003";
	listCmd.m_type = ImapCommandType::List;
	listCmd.m_args = {"/", "*"};
	std::string listResponse = dispatcher->Dispatch(listCmd);
	EXPECT_THAT(listResponse, testing::HasSubstr("NewName"));
	EXPECT_THAT(listResponse, testing::Not(testing::HasSubstr("OldName")));
}

TEST_F(CmdHandlerTests, HandleRename_FolderNotFound)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Rename;
	cmd.m_args = {"NonExistent", "NewName"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 NO No mailbox with specified name\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleRename_MissingArgs)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Rename;
	cmd.m_args = {"OldName"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Invalid arguments number\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCopy_Success)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Copy;
	cmd.m_args = {"1", "Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Copy completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCopy_MultipleMessages)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Copy;
	cmd.m_args = {"1:2", "Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Copy completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCopy_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Copy;
	cmd.m_args = {"1", "Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD No mailbox selected\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCopy_DestinationNotFound)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Copy;
	cmd.m_args = {"1", "NonExistentFolder"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD No such folder\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCopy_MissingArgs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Copy;
	cmd.m_args = {"1"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD Missing arguments\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleExpunge_Success_NoDeletedMessages)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Expunge;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Expunge completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleExpunge_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Expunge;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD No mailbox selected\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleExpunge_WithInvalidArgs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Expunge;
	cmd.m_args = {"invalid"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD Invalid arguments\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleLogout_Basic)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Logout;

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* BYE Logout\r\nA001 OK Logout completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleNoop_Basic)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Noop;

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 OK Noop completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCapability_NoAuthRequired)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Capability;

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* CAPABILITY IMAP4rev1\r\n";
	EXPECT_THAT(response, testing::HasSubstr(expected));
}

TEST_F(CmdHandlerTests, RequiresAuth_LoginNotRequired)
{
	EXPECT_FALSE(dispatcher->RequiresAuth(ImapCommandType::Login));
	EXPECT_FALSE(dispatcher->RequiresAuth(ImapCommandType::Logout));
	EXPECT_FALSE(dispatcher->RequiresAuth(ImapCommandType::Capability));
	EXPECT_FALSE(dispatcher->RequiresAuth(ImapCommandType::Noop));
}

TEST_F(CmdHandlerTests, RequiresAuth_SelectRequired)
{
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Select));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::List));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Lsub));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Status));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Fetch));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Store));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Create));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Delete));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Rename));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Copy));
	EXPECT_TRUE(dispatcher->RequiresAuth(ImapCommandType::Expunge));
}

TEST_F(CmdHandlerTests, Helper_LoginAndSelect)
{
	LoginAndSelect("alice", "INBOX");
	EXPECT_EQ(dispatcher->get_State(), SessionState::Selected);
}

TEST_F(CmdHandlerTests, Helper_Login)
{
	Login("alice");
	EXPECT_EQ(dispatcher->get_State(), SessionState::Authenticated);
}

TEST_F(CmdHandlerTests, HandleUidFetch_Success)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "FLAGS"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* 1 FETCH (UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("FLAGS"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK Uid Fetch completed\r\n"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_MultipleUIDs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1:3", "FLAGS"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("UID 2"));
	EXPECT_THAT(response, testing::HasSubstr("UID 3"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK Uid Fetch completed\r\n"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_InvalidUID)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"999", "FLAGS"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("A002 OK Uid Fetch completed\r\n"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "FLAGS"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 BAD No mailbox selected\r\n");
}

TEST_F(CmdHandlerTests, HandleUidFetch_MissingArgs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 BAD Missing arguments\r\n");
}

TEST_F(CmdHandlerTests, HandleUidStore_AddFlags)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidStore;
	cmd.m_args = {"1", "+FLAGS", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* 1 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("FLAGS (\\Seen"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK Uid Store completed"));
}

TEST_F(CmdHandlerTests, HandleUidStore_RemoveFlags)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidStore;
	cmd.m_args = {"1", "+FLAGS", "(\\Seen)"};
	dispatcher->Dispatch(cmd);

	cmd.m_tag = "A003";
	cmd.m_type = ImapCommandType::UidStore;
	cmd.m_args = {"1", "-FLAGS", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("A003 OK Uid Store completed\r\n"));
}

TEST_F(CmdHandlerTests, HandleUidStore_Silent)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidStore;
	cmd.m_args = {"1", "+FLAGS.SILENT", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 OK Uid Store completed\r\n");
	EXPECT_THAT(response, testing::Not(testing::HasSubstr("FETCH")));
}

TEST_F(CmdHandlerTests, HandleUidStore_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidStore;
	cmd.m_args = {"1", "+FLAGS", "(\\Seen)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 BAD No mailbox selected\r\n");
}

TEST_F(CmdHandlerTests, HandleUidStore_MissingArgs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidStore;
	cmd.m_args = {"1"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 BAD Missing arguments\r\n");
}

TEST_F(CmdHandlerTests, HandleUidCopy_Success)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidCopy;
	cmd.m_args = {"1", "Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 OK Uid Copy completed\r\n");
}

TEST_F(CmdHandlerTests, HandleUidCopy_MultipleMessages)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidCopy;
	cmd.m_args = {"1:3", "Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 OK Uid Copy completed\r\n");
}

TEST_F(CmdHandlerTests, HandleUidCopy_DestinationNotFound)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidCopy;
	cmd.m_args = {"1", "NonExistentFolder"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 BAD No such folder\r\n");
}

TEST_F(CmdHandlerTests, HandleUidCopy_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidCopy;
	cmd.m_args = {"1", "Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 BAD No mailbox selected\r\n");
}

TEST_F(CmdHandlerTests, HandleUidCopy_MissingArgs)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidCopy;
	cmd.m_args = {"1"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A002 BAD Missing arguments\r\n");
}

TEST_F(CmdHandlerTests, HandleClose_Success)
{
	LoginAndSelect("alice", "INBOX");
	ASSERT_EQ(dispatcher->get_State(), SessionState::Selected);

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Close;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A001 OK Close completed\r\n");
	EXPECT_EQ(dispatcher->get_State(), SessionState::Authenticated);
}

TEST_F(CmdHandlerTests, HandleClose_NoMailboxSelected)
{
	Login("alice");
	ASSERT_EQ(dispatcher->get_State(), SessionState::Authenticated);

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Close;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A001 BAD No mailbox selected\r\n");
}

TEST_F(CmdHandlerTests, HandleCheck_Success)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Check;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_EQ(response, "A001 OK Check completed\r\n");
}

TEST_F(CmdHandlerTests, HandleCheck_NoMailboxSelected)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Check;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD No mailbox selected\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleSubscribe_FolderNotFound)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Subscribe;
	cmd.m_args = {"NonExistentFolder"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Mailbox does not exist\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleSubscribe_MissingArgs)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Subscribe;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Invalid arguments number\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleUnsubscribe_MissingArgs)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Unsubscribe;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Invalid arguments number\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleClose_WithDeletedMessages)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand storeCmd;
	storeCmd.m_tag = "A001";
	storeCmd.m_type = ImapCommandType::Store;
	storeCmd.m_args = {"1", "+FLAGS", "(\\Deleted)"};
	dispatcher->Dispatch(storeCmd);

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Close;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Close completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleLogin_MissingArgs)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Login;
	cmd.m_args = {"alice"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Missing arguments\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleLogin_ExtraArgs)
{
	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Login;
	cmd.m_args = {"alice", "pass123", "extra"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Missing arguments\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleList_WildcardAsterisk)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::List;
	cmd.m_args = {"", "*"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* LIST (\\HasNoChildren) \"/\" \"INBOX\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleList_WildcardPercent)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::List;
	cmd.m_args = {"", "%"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* LIST (\\HasNoChildren) \"/\" \"INBOX\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleList_WithReference)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::List;
	cmd.m_args = {"/", "*"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* LIST (\\HasNoChildren) \"/\" \"INBOX\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleLsub_WildcardAsterisk)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Lsub;
	cmd.m_args = {"", "*"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* LSUB (\\HasNoChildren) \"/\" \"INBOX\"\r\n"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleStatus_Success_UIDNEXT)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"INBOX", "(UIDNEXT)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* STATUS INBOX (UIDNEXT 5)\r\nA001 OK Status completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStatus_Success_UIDVALIDITY)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"INBOX", "(UIDVALIDITY)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* STATUS INBOX (UIDVALIDITY 1)\r\nA001 OK Status completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStatus_Success_AllItems)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"INBOX", "(MESSAGES UNSEEN RECENT UIDNEXT UIDVALIDITY)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("MESSAGES 4"));
	EXPECT_THAT(response, testing::HasSubstr("UIDNEXT 5"));
	EXPECT_THAT(response, testing::HasSubstr("A001 OK"));
}

TEST_F(CmdHandlerTests, HandleStatus_EmptyMailbox)
{
	Login("alice");

	ImapCommand createCmd;
	createCmd.m_tag = "A001";
	createCmd.m_type = ImapCommandType::Create;
	createCmd.m_args = {"EmptyFolder"};
	dispatcher->Dispatch(createCmd);

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Status;
	cmd.m_args = {"EmptyFolder", "(MESSAGES UNSEEN)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* STATUS EmptyFolder (MESSAGES 0 UNSEEN 0)\r\nA002 OK Status completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleFetch_Success_BODY)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "BODY"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("* 1 FETCH ("));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_BODYSTRUCTURE)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "BODYSTRUCTURE"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODYSTRUCTURE"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_RFC822)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "RFC822"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("RFC822"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_RFC822HEADER)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "RFC822.HEADER"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("RFC822.HEADER"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_Success_RFC822TEXT)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "RFC822.TEXT"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("RFC822.TEXT"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_InvalidDataItem)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "INVALID"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD Invalid fetch attribute: INVALID\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleFetch_UID)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "UID"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* 1 FETCH (UID 1)\r\nA002 OK Fetch completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleStore_ReplaceFlags)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"1", "FLAGS", "(\\Seen \\Answered)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("FLAGS (\\Seen \\Answered"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleStore_InvalidFlags)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Store;
	cmd.m_args = {"1", "+FLAGS", "(\\InvalidFlag)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 BAD Unknown flag: \\InvalidFlag\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCreate_InvalidName)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Create;
	cmd.m_args = {""};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 NO Create failed";
	std::string actual = response.substr(0, expected.size());
	EXPECT_EQ(actual, expected);
}

TEST_F(CmdHandlerTests, HandleDelete_WithMessages)
{
	Login("alice");

	ImapCommand createCmd;
	createCmd.m_tag = "A001";
	createCmd.m_type = ImapCommandType::Create;
	createCmd.m_args = {"FolderWithMessages"};
	dispatcher->Dispatch(createCmd);

	ImapCommand selectCmd;
	selectCmd.m_tag = "A002";
	selectCmd.m_type = ImapCommandType::Select;
	selectCmd.m_args = {"FolderWithMessages"};
	dispatcher->Dispatch(selectCmd);

	ImapCommand deleteCmd;
	deleteCmd.m_tag = "A003";
	deleteCmd.m_type = ImapCommandType::Delete;
	deleteCmd.m_args = {"FolderWithMessages"};

	std::string response = dispatcher->Dispatch(deleteCmd);

	std::string expected = "A003 OK Delete completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleRename_AlreadyExists)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A001";
	cmd.m_type = ImapCommandType::Rename;
	cmd.m_args = {"INBOX", "Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A001 BAD Rename failed: renameFolder: folder 'Sent' already exists\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleCopy_ToSelf)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Copy;
	cmd.m_args = {"1", "INBOX"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Copy completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleExpunge_WithDeletedMessages)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand storeCmd;
	storeCmd.m_tag = "A001";
	storeCmd.m_type = ImapCommandType::Store;
	storeCmd.m_args = {"1", "+FLAGS", "(\\Deleted)"};
	dispatcher->Dispatch(storeCmd);

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Expunge;
	cmd.m_args = {};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("1 EXPUNGE"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_AllMacro)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "ALL"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_FastMacro)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "FAST"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_FullMacro)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "FULL"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_BODY)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "BODY"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidStore_ReplaceFlagsSilent)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidStore;
	cmd.m_args = {"1", "FLAGS.SILENT", "(\\Seen \\Answered)"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Uid Store completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleUidCopy_ToSelf)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidCopy;
	cmd.m_args = {"1", "INBOX"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "A002 OK Uid Copy completed\r\n";
	EXPECT_EQ(response, expected);
}

TEST_F(CmdHandlerTests, HandleFetch_BODYTEXT)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "BODY[TEXT]"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODY[TEXT]"));
	EXPECT_THAT(response, testing::HasSubstr("This is the body of the message"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_BODYHEADER)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "BODY[HEADER]"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODY[HEADER]"));
	EXPECT_THAT(response, testing::HasSubstr("From: alice@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("To: recipient@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("Subject: Hello Bob"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_BODY1_MIME)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"4", "BODY[1.MIME]"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODY[1.MIME]"));
	EXPECT_THAT(response, testing::HasSubstr("Content-Type: text/plain"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_BODY1)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"4", "BODY[1]"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODY[1]"));
	EXPECT_THAT(response, testing::HasSubstr("Plain text version"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_BODYPEEKHeaderFields)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "BODY.PEEK[HEADER.FIELDS (FROM TO SUBJECT)]"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODY.PEEK[HEADER.FIELDS"));
	EXPECT_THAT(response, testing::HasSubstr("From: alice@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("To: recipient@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("Subject: Hello Bob"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_MultipleDataItems)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1", "(FLAGS UID RFC822.SIZE)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("FLAGS"));
	EXPECT_THAT(response, testing::HasSubstr("UID"));
	EXPECT_THAT(response, testing::HasSubstr("RFC822.SIZE"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleFetch_SequenceRange)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Fetch;
	cmd.m_args = {"1:3", "FLAGS"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("1 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("2 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("3 FETCH"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_BODYSTRUCTURE)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "BODYSTRUCTURE"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODYSTRUCTURE"));
	EXPECT_THAT(response, testing::HasSubstr("\"text\" \"plain\""));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_RFC822)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "RFC822"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("RFC822"));
	EXPECT_THAT(response, testing::HasSubstr("From: alice@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_RFC822HEADER)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "RFC822.HEADER"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("RFC822.HEADER"));
	EXPECT_THAT(response, testing::HasSubstr("From: alice@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("Subject: Hello Bob"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_RFC822TEXT)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "RFC822.TEXT"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("RFC822.TEXT"));
	EXPECT_THAT(response, testing::HasSubstr("This is the body of the message"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_BODYTEXT)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "BODY[TEXT]"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODY[TEXT]"));
	EXPECT_THAT(response, testing::HasSubstr("This is the body of the message"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_BODYHEADER)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "BODY[HEADER]"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("BODY[HEADER]"));
	EXPECT_THAT(response, testing::HasSubstr("From: alice@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("To: recipient@test.com"));
	EXPECT_THAT(response, testing::HasSubstr("Subject: Hello Bob"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}

TEST_F(CmdHandlerTests, HandleUidFetch_MultipleDataItems)
{
	LoginAndSelect("alice", "INBOX");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::UidFetch;
	cmd.m_args = {"1", "(FLAGS RFC822.SIZE ENVELOPE)"};

	std::string response = dispatcher->Dispatch(cmd);

	EXPECT_THAT(response, testing::HasSubstr("UID 1"));
	EXPECT_THAT(response, testing::HasSubstr("FLAGS"));
	EXPECT_THAT(response, testing::HasSubstr("RFC822.SIZE"));
	EXPECT_THAT(response, testing::HasSubstr("ENVELOPE"));
	EXPECT_THAT(response, testing::HasSubstr("Hello Bob"));
	EXPECT_THAT(response, testing::HasSubstr("A002 OK"));
}