#include <gtest/gtest.h>

#include <atomic>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "../DataBaseManager.h"
#include "../Repository/MessageRepository.h"
#include "../Repository/UserRepository.h"

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

std::atomic<int> g_db_counter{0};

std::string uniqueDbPath() {
    auto tmp = std::filesystem::temp_directory_path();
    return (tmp / ("msg_repo_" + std::to_string(++g_db_counter) + ".db")).string();
}

void removeDb(const std::string& path) {
    for (const char* suffix : {"", "-wal", "-shm"})
        std::filesystem::remove(path + suffix);
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class MessageRepositoryTest : public ::testing::Test {
protected:
    std::string                        m_path;
    std::unique_ptr<DataBaseManager>   m_mgr;
    std::unique_ptr<UserRepository>    m_user_repo;
    std::unique_ptr<MessageRepository> m_msg_repo;

    int64_t m_user_id{0};
    int64_t m_inbox_id{0};

    void SetUp() override {
        m_path      = uniqueDbPath();
        auto scheme_path = (std::filesystem::path(__FILE__).parent_path().parent_path() / "scheme" / "001_init_scheme.sql").string();
        m_mgr       = std::make_unique<DataBaseManager>(m_path, scheme_path);
        ASSERT_TRUE(m_mgr->isConnected()) << "DB failed to open: " << m_path;
        m_user_repo = std::make_unique<UserRepository>(*m_mgr);
        m_msg_repo  = std::make_unique<MessageRepository>(*m_mgr);

        // Register a test user (UserRepository may create default folders on registration)
        User u;
        u.username = "testuser";
        ASSERT_TRUE(m_user_repo->registerUser(u, "password"));
        m_user_id = *u.id;

        // Obtain the INBOX folder (created by registerUser) or create one manually
        auto inbox = m_msg_repo->findFolderByName(m_user_id, "INBOX");
        if (inbox.has_value()) {
            m_inbox_id = *inbox->id;
        } else {
            Folder f;
            f.user_id = m_user_id;
            f.name    = "INBOX";
            ASSERT_TRUE(m_msg_repo->createFolder(f));
            m_inbox_id = *f.id;
        }
    }

    void TearDown() override {
        m_msg_repo.reset();
        m_user_repo.reset();
        m_mgr.reset();
        removeDb(m_path);
    }

    // ── Factory helpers ──────────────────────────────────────────────────────

    Message buildMessage(const std::string& from    = "sender@example.com",
                         const std::string& subject = "Test Subject") const {
        Message m;
        m.user_id       = m_user_id;
        m.folder_id     = m_inbox_id;
        m.uid           = 0;                     // assigned by repository
        m.raw_file_path = "/mnt/mail/test.eml";
        m.size_bytes    = 512;
        m.from_address  = from;
        m.subject       = subject;
        m.internal_date = "2024-06-01T10:00:00Z";
        m.is_recent     = true;
        return m;
    }

    Folder buildFolder(const std::string&        name,
                       std::optional<int64_t>    parent = std::nullopt) const {
        Folder f;
        f.user_id   = m_user_id;
        f.name      = name;
        f.parent_id = parent;
        return f;
    }

    // Delivers a message and asserts success; returns the populated message
    Message deliver(const std::string& from    = "sender@example.com",
                    const std::string& subject = "Test Subject",
                    int64_t            folder  = 0) {
        Message m = buildMessage(from, subject);
        if (folder != 0) m.folder_id = folder;
        int64_t target = (folder != 0) ? folder : m_inbox_id;
        EXPECT_TRUE(m_msg_repo->deliver(m, target));
        return m;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Folder CRUD
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, CreateFolder_AssignsPositiveID) {
    Folder f = buildFolder("CustomFolder");
    ASSERT_TRUE(m_msg_repo->createFolder(f));
    ASSERT_TRUE(f.id.has_value());
    EXPECT_GT(*f.id, 0);
}

TEST_F(MessageRepositoryTest, FindFolderByID_ExistingFolder_ReturnsCorrectFolder) {
    Folder f = buildFolder("TestFolder");
    ASSERT_TRUE(m_msg_repo->createFolder(f));

    auto found = m_msg_repo->findFolderByID(*f.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name,    "TestFolder");
    EXPECT_EQ(found->user_id, m_user_id);
}

TEST_F(MessageRepositoryTest, FindFolderByID_NonExistent_ReturnsNullopt) {
    EXPECT_FALSE(m_msg_repo->findFolderByID(999999).has_value());
}

TEST_F(MessageRepositoryTest, FindFolderByName_ExistingFolder_ReturnsFolder) {
    Folder f = buildFolder("CustomFolder");
    ASSERT_TRUE(m_msg_repo->createFolder(f));

    auto found = m_msg_repo->findFolderByName(m_user_id, "CustomFolder");
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(*found->id, *f.id);
}

TEST_F(MessageRepositoryTest, FindFolderByName_NonExistent_ReturnsNullopt) {
    EXPECT_FALSE(m_msg_repo->findFolderByName(m_user_id, "NoSuchFolder").has_value());
}

TEST_F(MessageRepositoryTest, FindFolderByName_IsolatedPerUser) {
    // Create a second user with a folder of the same name
    User u2;
    u2.username = "otheruser";
    ASSERT_TRUE(m_user_repo->registerUser(u2, "pass"));

    Folder f;
    f.user_id = *u2.id;
    f.name    = "SharedName";
    ASSERT_TRUE(m_msg_repo->createFolder(f));

    // user 1 must not see user 2's folder
    EXPECT_FALSE(m_msg_repo->findFolderByName(m_user_id, "SharedName").has_value());
}

TEST_F(MessageRepositoryTest, FindFoldersByUser_ReturnsAtLeastCreatedFolders) {
    Folder sent = buildFolder("MyFolder1");
    m_msg_repo->createFolder(sent);
    Folder trash = buildFolder("MyFolder2");
    m_msg_repo->createFolder(trash);

    auto folders = m_msg_repo->findFoldersByUser(m_user_id);
    EXPECT_GE(folders.size(), 2u);
}

TEST_F(MessageRepositoryTest, RenameFolder_ChangesNameInDB) {
    Folder f = buildFolder("OldName");
    ASSERT_TRUE(m_msg_repo->createFolder(f));
    ASSERT_TRUE(m_msg_repo->renameFolder(*f.id, "NewName"));

    auto found = m_msg_repo->findFolderByID(*f.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->name, "NewName");
}

TEST_F(MessageRepositoryTest, DeleteFolder_FolderNoLongerFindable) {
    Folder f = buildFolder("ToDelete");
    ASSERT_TRUE(m_msg_repo->createFolder(f));
    ASSERT_TRUE(m_msg_repo->deleteFolder(*f.id));
    EXPECT_FALSE(m_msg_repo->findFolderByID(*f.id).has_value());
}

TEST_F(MessageRepositoryTest, SetSubscribed_UpdatesFlag) {
    ASSERT_TRUE(m_msg_repo->setSubscribed(m_inbox_id, true));
    EXPECT_TRUE(m_msg_repo->findFolderByID(m_inbox_id)->is_subscribed);

    ASSERT_TRUE(m_msg_repo->setSubscribed(m_inbox_id, false));
    EXPECT_FALSE(m_msg_repo->findFolderByID(m_inbox_id)->is_subscribed);
}

TEST_F(MessageRepositoryTest, FindFoldersByParent_ReturnsChildFolders) {
    Folder parent = buildFolder("Parent");
    ASSERT_TRUE(m_msg_repo->createFolder(parent));

    Folder child1 = buildFolder("Child1", *parent.id);
    Folder child2 = buildFolder("Child2", *parent.id);
    ASSERT_TRUE(m_msg_repo->createFolder(child1));
    ASSERT_TRUE(m_msg_repo->createFolder(child2));

    auto children = m_msg_repo->findFoldersByParent(*parent.id);
    EXPECT_EQ(children.size(), 2u);
}

TEST_F(MessageRepositoryTest, FindFoldersByParent_NoChildren_ReturnsEmpty) {
    Folder f = buildFolder("Leaf");
    ASSERT_TRUE(m_msg_repo->createFolder(f));
    EXPECT_TRUE(m_msg_repo->findFoldersByParent(*f.id).empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Message delivery & lookup
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, Deliver_AssignsIDAndUID) {
    Message m = buildMessage();
    ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m.id.has_value());
    EXPECT_GT(*m.id,  0);
    EXPECT_GT(m.uid,  0);
}

TEST_F(MessageRepositoryTest, Deliver_MultipleMessages_UIDs_AreStrictlyIncreasing) {
    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));
    Message m3 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m3, m_inbox_id));

    EXPECT_LT(m1.uid, m2.uid);
    EXPECT_LT(m2.uid, m3.uid);
}

TEST_F(MessageRepositoryTest, FindByID_ExistingMessage_ReturnsCorrectData) {
    Message m = buildMessage("bob@example.com", "Hello");
    ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->from_address, "bob@example.com");
    ASSERT_TRUE(found->subject.has_value());
    EXPECT_EQ(*found->subject, "Hello");
    EXPECT_EQ(found->user_id,  m_user_id);
}

TEST_F(MessageRepositoryTest, FindByID_NonExistent_ReturnsNullopt) {
    EXPECT_FALSE(m_msg_repo->findByID(999999).has_value());
}

TEST_F(MessageRepositoryTest, FindByUID_ReturnsCorrectMessage) {
    Message m = buildMessage();
    ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    auto found = m_msg_repo->findByUID(m_inbox_id, m.uid);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->id, m.id);
}

TEST_F(MessageRepositoryTest, FindByUID_NonExistent_ReturnsNullopt) {
    EXPECT_FALSE(m_msg_repo->findByUID(m_inbox_id, 999999).has_value());
}

TEST_F(MessageRepositoryTest, FindByFolder_ReturnsOnlyFolderMessages) {
    Folder other = buildFolder("Other");
    ASSERT_TRUE(m_msg_repo->createFolder(other));

    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));
    Message m3 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m3, *other.id));

    auto inbox_msgs = m_msg_repo->findByFolder(m_inbox_id);
    EXPECT_EQ(inbox_msgs.size(), 2u);

    auto other_msgs = m_msg_repo->findByFolder(*other.id);
    EXPECT_EQ(other_msgs.size(), 1u);
}

TEST_F(MessageRepositoryTest, FindByUser_ReturnsAllMessagesAcrossFolders) {
    Folder sent = buildFolder("FolderForMessages");
    ASSERT_TRUE(m_msg_repo->createFolder(sent));

    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, *sent.id));

    auto msgs = m_msg_repo->findByUser(m_user_id);
    EXPECT_GE(msgs.size(), 2u);
}

TEST_F(MessageRepositoryTest, FindByFolder_Pagination) {
    for (int i = 0; i < 5; ++i) {
        Message m = buildMessage();
        m_msg_repo->deliver(m, m_inbox_id);
    }

    auto page1 = m_msg_repo->findByFolder(m_inbox_id, 2, 0);
    auto page2 = m_msg_repo->findByFolder(m_inbox_id, 2, 2);
    auto page3 = m_msg_repo->findByFolder(m_inbox_id, 2, 4);

    EXPECT_EQ(page1.size(), 2u);
    EXPECT_EQ(page2.size(), 2u);
    EXPECT_EQ(page3.size(), 1u);

    // No duplicate IDs across pages
    for (const auto& a : page1)
        for (const auto& b : page2)
            EXPECT_NE(a.id, b.id);
}

TEST_F(MessageRepositoryTest, Append_InsertsMessagePreservingFlags) {
    Message m = buildMessage();
    m.is_seen  = true;
    m.is_draft = true;
    m.is_recent = false;

    ASSERT_TRUE(m_msg_repo->append(m, m_inbox_id));
    ASSERT_TRUE(m.id.has_value());

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_draft);
}

TEST_F(MessageRepositoryTest, SaveToFolder_InsertsMessage) {
    Folder drafts = buildFolder("SaveTestFolder");
    ASSERT_TRUE(m_msg_repo->createFolder(drafts));

    Message m = buildMessage("me@me.com", "Draft");
    m.is_draft = true;
    ASSERT_TRUE(m_msg_repo->saveToFolder(m, *drafts.id));
    ASSERT_TRUE(m.id.has_value());

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, *drafts.id);
}

// ─────────────────────────────────────────────────────────────────────────────
// Mark / flag operations
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, MarkSeen_TogglesSeenFlag) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    EXPECT_FALSE(m_msg_repo->findByID(*m.id)->is_seen);

    ASSERT_TRUE(m_msg_repo->markSeen(*m.id, true));
    EXPECT_TRUE(m_msg_repo->findByID(*m.id)->is_seen);

    ASSERT_TRUE(m_msg_repo->markSeen(*m.id, false));
    EXPECT_FALSE(m_msg_repo->findByID(*m.id)->is_seen);
}

TEST_F(MessageRepositoryTest, MarkDeleted_TogglesDeletedFlag) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    ASSERT_TRUE(m_msg_repo->markDeleted(*m.id, true));
    EXPECT_TRUE(m_msg_repo->findByID(*m.id)->is_deleted);

    ASSERT_TRUE(m_msg_repo->markDeleted(*m.id, false));
    EXPECT_FALSE(m_msg_repo->findByID(*m.id)->is_deleted);
}

TEST_F(MessageRepositoryTest, MarkFlagged_TogglesFlaggedFlag) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->markFlagged(*m.id, true));
    EXPECT_TRUE(m_msg_repo->findByID(*m.id)->is_flagged);
}

TEST_F(MessageRepositoryTest, MarkAnswered_TogglesAnsweredFlag) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->markAnswered(*m.id, true));
    EXPECT_TRUE(m_msg_repo->findByID(*m.id)->is_answered);
}

TEST_F(MessageRepositoryTest, MarkDraft_TogglesDraftFlag) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->markDraft(*m.id, true));
    EXPECT_TRUE(m_msg_repo->findByID(*m.id)->is_draft);
}

TEST_F(MessageRepositoryTest, UpdateFlags_SetsAllFlagsAtOnce) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    // seen=T, deleted=F, draft=T, answered=F, flagged=T, recent=F
    ASSERT_TRUE(m_msg_repo->updateFlags(*m.id, true, false, true, false, true, false));

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_FALSE(found->is_deleted);
    EXPECT_TRUE(found->is_draft);
    EXPECT_FALSE(found->is_answered);
    EXPECT_TRUE(found->is_flagged);
    EXPECT_FALSE(found->is_recent);
}

TEST_F(MessageRepositoryTest, UpdateFlags_AllFalse_ClearsEverything) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    m_msg_repo->markSeen(*m.id, true);
    m_msg_repo->markFlagged(*m.id, true);

    ASSERT_TRUE(m_msg_repo->updateFlags(*m.id, false, false, false, false, false, false));

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_FALSE(found->is_flagged);
}

TEST_F(MessageRepositoryTest, SetFlags_IMAPStyleSeen_SetsCorrectFlag) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->setFlags(*m.id, {"\\Seen"}));

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_FALSE(found->is_flagged);
    EXPECT_FALSE(found->is_deleted);
}

TEST_F(MessageRepositoryTest, SetFlags_MultipleIMAPFlags_SetsAll) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->setFlags(*m.id, {"\\Seen", "\\Flagged", "\\Answered"}));

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_TRUE(found->is_seen);
    EXPECT_TRUE(found->is_flagged);
    EXPECT_TRUE(found->is_answered);
    EXPECT_FALSE(found->is_deleted);
    EXPECT_FALSE(found->is_draft);
}

TEST_F(MessageRepositoryTest, SetFlags_EmptyList_ClearsAllFlags) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    m_msg_repo->markSeen(*m.id, true);
    m_msg_repo->markFlagged(*m.id, true);

    ASSERT_TRUE(m_msg_repo->setFlags(*m.id, {}));

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_FALSE(found->is_seen);
    EXPECT_FALSE(found->is_flagged);
}

// ─────────────────────────────────────────────────────────────────────────────
// Filtered queries
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, FindUnseen_ReturnsOnlyUnseenMessages) {
    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));
    m_msg_repo->markSeen(*m1.id, true);

    auto unseen = m_msg_repo->findUnseen(m_inbox_id);
    ASSERT_EQ(unseen.size(), 1u);
    EXPECT_EQ(unseen[0].id, m2.id);
}

TEST_F(MessageRepositoryTest, FindUnseen_AllSeen_ReturnsEmpty) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    m_msg_repo->markSeen(*m.id, true);
    EXPECT_TRUE(m_msg_repo->findUnseen(m_inbox_id).empty());
}

TEST_F(MessageRepositoryTest, FindDeleted_ReturnsOnlyDeletedMessages) {
    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));
    m_msg_repo->markDeleted(*m1.id, true);

    auto deleted = m_msg_repo->findDeleted(m_inbox_id);
    ASSERT_EQ(deleted.size(), 1u);
    EXPECT_EQ(deleted[0].id, m1.id);
}

TEST_F(MessageRepositoryTest, FindFlagged_ReturnsOnlyFlaggedMessages) {
    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));
    m_msg_repo->markFlagged(*m2.id, true);

    auto flagged = m_msg_repo->findFlagged(m_inbox_id);
    ASSERT_EQ(flagged.size(), 1u);
    EXPECT_EQ(flagged[0].id, m2.id);
}

TEST_F(MessageRepositoryTest, Search_FindsMessageByFromAddress) {
    Message m1 = buildMessage("alice@example.com", "Hi");
    Message m2 = buildMessage("bob@example.com",   "Hello");
    ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));

    auto results = m_msg_repo->search(m_user_id, "alice");
    ASSERT_GE(results.size(), 1u);

    bool found_alice = false;
    for (const auto& r : results)
        if (r.from_address == "alice@example.com") found_alice = true;
    EXPECT_TRUE(found_alice);
}

TEST_F(MessageRepositoryTest, Search_FindsMessageBySubject) {
    Message m = buildMessage("x@x.com", "UniqueSubjectXYZ123");
    ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    auto results = m_msg_repo->search(m_user_id, "UniqueSubjectXYZ123");
    EXPECT_GE(results.size(), 1u);
}

TEST_F(MessageRepositoryTest, Search_NoMatch_ReturnsEmpty) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    auto results = m_msg_repo->search(m_user_id, "zzz_no_match_zzz");
    EXPECT_TRUE(results.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// Move, Copy, Expunge, HardDelete
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, MoveToFolder_ChangesMessageFolderAndAssignsNewUID) {
    Folder dest = buildFolder("Archive");
    ASSERT_TRUE(m_msg_repo->createFolder(dest));
    
    // Deliver one message to Archive first so its next_uid > 1
    Message dummy = buildMessage("dummy@example.com", "Dummy");
    ASSERT_TRUE(m_msg_repo->deliver(dummy, *dest.id));

    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    int64_t old_uid = m.uid;
    ASSERT_TRUE(m_msg_repo->moveToFolder(*m.id, *dest.id));

    auto found = m_msg_repo->findByID(*m.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->folder_id, *dest.id);
    // UID in new folder should differ from UID in old folder
    EXPECT_NE(found->uid, old_uid);
}

TEST_F(MessageRepositoryTest, MoveToFolder_MessageNoLongerInSourceFolder) {
    Folder dest = buildFolder("Moved_Dest");
    ASSERT_TRUE(m_msg_repo->createFolder(dest));

    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->moveToFolder(*m.id, *dest.id));

    auto inbox_msgs = m_msg_repo->findByFolder(m_inbox_id);
    for (const auto& msg : inbox_msgs)
        EXPECT_NE(msg.id, m.id);
}

TEST_F(MessageRepositoryTest, Copy_CreatesNewMessageInTargetFolder) {
    Folder dest = buildFolder("Copy_Dest");
    ASSERT_TRUE(m_msg_repo->createFolder(dest));

    Message m = buildMessage("orig@example.com", "Original");
    ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    auto copied = m_msg_repo->copy(*m.id, *dest.id);
    ASSERT_TRUE(copied.has_value());
    EXPECT_NE(copied->id, m.id);                       // different row
    EXPECT_EQ(copied->folder_id, *dest.id);
    EXPECT_EQ(copied->from_address, "orig@example.com");
}

TEST_F(MessageRepositoryTest, Copy_OriginalRemainsInSourceFolder) {
    Folder dest = buildFolder("Copy_Dest2");
    ASSERT_TRUE(m_msg_repo->createFolder(dest));

    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    m_msg_repo->copy(*m.id, *dest.id);

    EXPECT_TRUE(m_msg_repo->findByID(*m.id).has_value());
    auto inbox_msgs = m_msg_repo->findByFolder(m_inbox_id);
    bool original_still_there = false;
    for (const auto& msg : inbox_msgs)
        if (msg.id == m.id) original_still_there = true;
    EXPECT_TRUE(original_still_there);
}

TEST_F(MessageRepositoryTest, Expunge_DeletesOnlyMarkedDeletedMessages) {
    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));
    Message m3 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m3, m_inbox_id));

    m_msg_repo->markDeleted(*m1.id, true);
    m_msg_repo->markDeleted(*m3.id, true);
    // m2 is NOT deleted

    ASSERT_TRUE(m_msg_repo->expunge(m_inbox_id));

    EXPECT_FALSE(m_msg_repo->findByID(*m1.id).has_value());
    EXPECT_TRUE (m_msg_repo->findByID(*m2.id).has_value());
    EXPECT_FALSE(m_msg_repo->findByID(*m3.id).has_value());
}

TEST_F(MessageRepositoryTest, Expunge_NoDeletedMessages_NoEffect) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->expunge(m_inbox_id));
    EXPECT_TRUE(m_msg_repo->findByID(*m.id).has_value());
}

TEST_F(MessageRepositoryTest, HardDelete_RemovesMessageFromDB) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->hardDelete(*m.id));
    EXPECT_FALSE(m_msg_repo->findByID(*m.id).has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Recipients
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, AddRecipient_AssignsID) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    Recipient r;
    r.message_id = *m.id;
    r.address    = "to@example.com";
    r.type       = RecipientType::To;
    ASSERT_TRUE(m_msg_repo->addRecipient(r));
    ASSERT_TRUE(r.id.has_value());
    EXPECT_GT(*r.id, 0);
}

TEST_F(MessageRepositoryTest, FindRecipientsByMessage_ReturnsAllTypes) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));

    Recipient r1; r1.message_id = *m.id; r1.address = "to@e.com";      r1.type = RecipientType::To;
    Recipient r2; r2.message_id = *m.id; r2.address = "cc@e.com";      r2.type = RecipientType::Cc;
    Recipient r3; r3.message_id = *m.id; r3.address = "bcc@e.com";     r3.type = RecipientType::Bcc;
    Recipient r4; r4.message_id = *m.id; r4.address = "reply@e.com";   r4.type = RecipientType::ReplyTo;

    ASSERT_TRUE(m_msg_repo->addRecipient(r1));
    ASSERT_TRUE(m_msg_repo->addRecipient(r2));
    ASSERT_TRUE(m_msg_repo->addRecipient(r3));
    ASSERT_TRUE(m_msg_repo->addRecipient(r4));

    auto recs = m_msg_repo->findRecipientsByMessage(*m.id);
    EXPECT_EQ(recs.size(), 4u);
}

TEST_F(MessageRepositoryTest, FindRecipientByID_ReturnsCorrectRecipient) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    Recipient r; r.message_id = *m.id; r.address = "found@e.com"; r.type = RecipientType::To;
    ASSERT_TRUE(m_msg_repo->addRecipient(r));

    auto found = m_msg_repo->findRecipientByID(*r.id);
    ASSERT_TRUE(found.has_value());
    EXPECT_EQ(found->address, "found@e.com");
}

TEST_F(MessageRepositoryTest, RemoveRecipient_RecipientNoLongerFindable) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    Recipient r; r.message_id = *m.id; r.address = "rm@e.com"; r.type = RecipientType::To;
    ASSERT_TRUE(m_msg_repo->addRecipient(r));

    ASSERT_TRUE(m_msg_repo->removeRecipient(*r.id));
    EXPECT_FALSE(m_msg_repo->findRecipientByID(*r.id).has_value());
}

TEST_F(MessageRepositoryTest, FindRecipientsByMessage_EmptyForNewMessage) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    EXPECT_TRUE(m_msg_repo->findRecipientsByMessage(*m.id).empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// UID counter & Recent flag
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, IncrementNextUID_IncrementsCounter) {
    auto before = m_msg_repo->findFolderByID(m_inbox_id);
    ASSERT_TRUE(before.has_value());
    int64_t uid_before = before->next_uid;

    ASSERT_TRUE(m_msg_repo->incrementNextUID(m_inbox_id));

    auto after = m_msg_repo->findFolderByID(m_inbox_id);
    ASSERT_TRUE(after.has_value());
    EXPECT_EQ(after->next_uid, uid_before + 1);
}

TEST_F(MessageRepositoryTest, ClearRecentByFolder_ClearsRecentFlagOnAllMessages) {
    Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, m_inbox_id));
    Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, m_inbox_id));

    // Both should be recent after delivery
    EXPECT_TRUE(m_msg_repo->findByID(*m1.id)->is_recent);
    EXPECT_TRUE(m_msg_repo->findByID(*m2.id)->is_recent);

    ASSERT_TRUE(m_msg_repo->clearRecentByFolder(m_inbox_id));

    EXPECT_FALSE(m_msg_repo->findByID(*m1.id)->is_recent);
    EXPECT_FALSE(m_msg_repo->findByID(*m2.id)->is_recent);
}

TEST_F(MessageRepositoryTest, ClearRecentByFolder_DoesNotAffectOtherFolders) {
    Folder other = buildFolder("Other");
    ASSERT_TRUE(m_msg_repo->createFolder(other));

    Message m_inbox = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m_inbox, m_inbox_id));
    Message m_other = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m_other, *other.id));

    ASSERT_TRUE(m_msg_repo->clearRecentByFolder(m_inbox_id));

    EXPECT_FALSE(m_msg_repo->findByID(*m_inbox.id)->is_recent);
    EXPECT_TRUE (m_msg_repo->findByID(*m_other.id)->is_recent); // untouched
}

TEST_F(MessageRepositoryTest, CloseFolder_ClearsRecentMessages) {
    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, m_inbox_id));
    ASSERT_TRUE(m_msg_repo->closeFolder(m_inbox_id));
    EXPECT_FALSE(m_msg_repo->findByID(*m.id)->is_recent);
}

// ─────────────────────────────────────────────────────────────────────────────
// Cross-entity integrity
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(MessageRepositoryTest, DeleteFolder_AlsoRemovesMessages) {
    Folder tmp = buildFolder("Temp");
    ASSERT_TRUE(m_msg_repo->createFolder(tmp));

    Message m = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m, *tmp.id));
    int64_t msg_id = *m.id;

    ASSERT_TRUE(m_msg_repo->deleteFolder(*tmp.id));

    // Message should be gone once its parent folder is deleted
    EXPECT_FALSE(m_msg_repo->findByID(msg_id).has_value());
}

TEST_F(MessageRepositoryTest, MultipleDelivers_UIDsUniquePerFolder) {
    Folder f1 = buildFolder("F1");
    Folder f2 = buildFolder("F2");
    ASSERT_TRUE(m_msg_repo->createFolder(f1));
    ASSERT_TRUE(m_msg_repo->createFolder(f2));

    std::vector<int64_t> uids_f1, uids_f2;
    for (int i = 0; i < 3; ++i) {
        Message m1 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m1, *f1.id));
        Message m2 = buildMessage(); ASSERT_TRUE(m_msg_repo->deliver(m2, *f2.id));
        uids_f1.push_back(m1.uid);
        uids_f2.push_back(m2.uid);
    }

    // UIDs within each folder must be unique
    for (size_t i = 0; i < uids_f1.size(); ++i)
        for (size_t j = i + 1; j < uids_f1.size(); ++j)
            EXPECT_NE(uids_f1[i], uids_f1[j]);

    for (size_t i = 0; i < uids_f2.size(); ++i)
        for (size_t j = i + 1; j < uids_f2.size(); ++j)
            EXPECT_NE(uids_f2[i], uids_f2[j]);
}