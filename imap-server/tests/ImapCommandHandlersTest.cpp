#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <memory>

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

#ifndef SCHEMA_PATH
#define SCHEMA_PATH "../../database/scheme/001_init_scheme.sql"
#endif

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
		db = std::make_unique<DataBaseManager>(":memory:", SCHEMA_PATH);
		userDal = std::make_unique<UserDAL>(db->getDB());
		userRepo = std::make_unique<UserRepository>(db->getDB(), *userDal);
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

		userRepo->registerUser(alice, pass);
		userRepo->registerUser(bob, pass);
		userRepo->registerUser(charlie, pass);
		userRepo->registerUser(dave, pass);

		FolderDAL folderDal(db->getDB());

		auto bobSpam = Folder{std::nullopt, 2, "Spam", 1, true};
		auto bobWork = Folder{std::nullopt, 2, "Work", 1, true};
		auto charlieFamily = Folder{std::nullopt, 3, "Family", 1, true};
		auto aliceArchive = Folder{std::nullopt, 1, "Archive", 1, true};

		folderDal.insert(bobSpam);
		folderDal.insert(bobWork);
		folderDal.insert(charlieFamily);
		folderDal.insert(aliceArchive);

		auto aliceInbox = folderDal.findByName(1, "INBOX");
		auto aliceSent = folderDal.findByName(1, "Sent");
		auto bobInbox = folderDal.findByName(2, "INBOX");
		auto bobWorkFolder = folderDal.findByName(2, "Work");
		auto charlieInbox = folderDal.findByName(3, "INBOX");
		auto charlieFamilyFolder = folderDal.findByName(3, "Family");

		auto addMsg = [&](int64_t userId, int64_t folderId, const std::string& from, const std::string& subject,
						  bool flagged, bool answered)
		{
			Message msg;
			msg.user_id = userId;
			msg.from_address = from;
			msg.subject = subject;
			msg.raw_file_path = "/tmp/test_msg_" + subject + ".eml";
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

		addMsg(1, aliceInbox->id.value(), "alice@test.com", "Hello Bob", false, false);
		addMsg(1, aliceInbox->id.value(), "alice@test.com", "Re: Project", true, true);
		addMsg(1, aliceInbox->id.value(), "alice@test.com", "Third message", false, false);
		addMsg(1, aliceSent->id.value(), "alice@test.com", "Hi Bob", false, false);

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

	std::string expected = "* FLAGS (\\Seen \\Answered \\Flagged \\Draft \\Recent)\r\n"
						   "* 3 EXISTS\r\n"
						   "* 3 RECENT\r\n"
						   "A002 OK [READ-WRITE] Select completed\r\n";
	EXPECT_EQ(response, expected);
	EXPECT_EQ(dispatcher->get_State(), SessionState::Selected);
	EXPECT_EQ(dispatcher->get_MailboxState().m_name, "INBOX");
	EXPECT_EQ(dispatcher->get_MailboxState().m_exists, 3);
}

TEST_F(CmdHandlerTests, HandleSelect_SuccessDifferentFolder)
{
	Login("alice");

	ImapCommand cmd;
	cmd.m_tag = "A002";
	cmd.m_type = ImapCommandType::Select;
	cmd.m_args = {"Sent"};

	std::string response = dispatcher->Dispatch(cmd);

	std::string expected = "* FLAGS (\\Seen \\Answered \\Flagged \\Draft \\Recent)\r\n"
						   "* 1 EXISTS\r\n"
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

	std::string expected = "* STATUS INBOX (MESSAGES 3)\r\nA001 OK Status completed\r\n";
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

	EXPECT_THAT(response, testing::HasSubstr("MESSAGES 3"));
	EXPECT_THAT(response, testing::HasSubstr("UNSEEN 3"));
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

	EXPECT_THAT(response, testing::HasSubstr("RECENT 3"));
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

	EXPECT_THAT(response, testing::HasSubstr("FLAGS ()"));

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

	std::string expected = "* 3 FETCH (FLAGS (\\Seen))\r\nA002 OK Store completed\r\n";
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

	std::string expected = "* 1 FETCH (FLAGS (\\Seen \\Flagged))\r\nA002 OK Store completed\r\n";
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

	std::string expected = "* 1 FETCH (FLAGS ())\r\nA002 OK Store completed\r\n";
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

	std::string expected = "A001 BAD Create failed";
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

	std::string expected = "A001 BAD Invalid parametrs number\r\n";
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

	std::string expected = "A001 OK Delete completed\r\n";
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

	std::string expected =
		"* CAPABILITY IMAP4rev1 LOGIN LOGOUT SELECT LIST LSUB STATUS FETCH STORE CREATE DELETE RENAME COPY EXPUNGE\r\n";
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