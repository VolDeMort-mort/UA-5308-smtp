#include <gtest/gtest.h>
#include <fstream>
#include <sstream>

#include "../Repository/MessageRepository.h"
#include "../DataBaseManager.h"

class MessageRepositoryTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_db = new DataBaseManager(":memory:", SCHEMA_PATH);
        m_repo = new MessageRepository(*m_db);

        sqlite3_exec(m_db->getDB(),
            "INSERT INTO users   (id, username, password_hash) VALUES (1, 'alice', 'hash');"
            "INSERT INTO users   (id, username, password_hash) VALUES (2, 'bob',   'hash');"
            "INSERT INTO folders (id, user_id, name) VALUES (1, 1, 'Inbox');"
            "INSERT INTO folders (id, user_id, name) VALUES (2, 1, 'Archive');",
            nullptr, nullptr, nullptr);
    }

    void TearDown() override
    {
        delete m_repo;
        delete m_db;
    }

    Message makeDraft(const std::string& subject = "Subject",
                      const std::string& receiver = "bob@example.com")
    {
        Message msg;
        msg.user_id = 1;
        msg.folder_id = 1;
        msg.subject = subject;
        msg.receiver = receiver;
        msg.body = "Body.";
        return msg;
    }

    DataBaseManager*  m_db   = nullptr;
    MessageRepository* m_repo = nullptr;
};

TEST_F(MessageRepositoryTest, SaveDraft_SetsID)
{
    auto msg = makeDraft();
    EXPECT_TRUE(m_repo->saveDraft(msg));
    EXPECT_TRUE(msg.id.has_value());
}

TEST_F(MessageRepositoryTest, SaveDraft_ForcesDraftAndSeen)
{
    auto msg = makeDraft(); msg.status = MessageStatus::Sent; msg.is_seen = false;
    m_repo->saveDraft(msg);

    auto f = m_repo->findByID(msg.id.value());
    EXPECT_EQ(f->status, MessageStatus::Draft);
    EXPECT_TRUE(f->is_seen);
}

TEST_F(MessageRepositoryTest, EditDraft_UpdatesContent)
{
    auto msg = makeDraft("Original"); m_repo->saveDraft(msg);
    msg.subject = "Edited";
    EXPECT_TRUE(m_repo->editDraft(msg));
    EXPECT_EQ(m_repo->findByID(msg.id.value())->subject, "Edited");
}

TEST_F(MessageRepositoryTest, EditDraft_RejectsNonDraft)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg); m_repo->send(msg);
    msg.subject = "Nope";
    EXPECT_FALSE(m_repo->editDraft(msg));
}

TEST_F(MessageRepositoryTest, Send_TransitionsDraftToSent)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    EXPECT_TRUE(m_repo->send(msg));
    EXPECT_EQ(m_repo->findByID(msg.id.value())->status, MessageStatus::Sent);
}

TEST_F(MessageRepositoryTest, Send_RejectsAlreadySent)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg); m_repo->send(msg);
    EXPECT_FALSE(m_repo->send(msg));
}

TEST_F(MessageRepositoryTest, Send_RejectsNonDraft)
{
    auto msg = makeDraft(); m_repo->setDelivered(msg);
    EXPECT_FALSE(m_repo->send(msg));
}

TEST_F(MessageRepositoryTest, SetDelivered_ForcesReceivedAndUnseen)
{
    auto msg = makeDraft(); msg.is_seen = true;
    m_repo->setDelivered(msg);

    auto f = m_repo->findByID(msg.id.value());
    EXPECT_EQ(f->status, MessageStatus::Received);
    EXPECT_FALSE(f->is_seen);
}

TEST_F(MessageRepositoryTest, MarkSeen_Toggles)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    m_repo->markSeen(msg.id.value(), true);
    EXPECT_TRUE(m_repo->findByID(msg.id.value())->is_seen);
    m_repo->markSeen(msg.id.value(), false);
    EXPECT_FALSE(m_repo->findByID(msg.id.value())->is_seen);
}

TEST_F(MessageRepositoryTest, MarkStarred_Toggles)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    m_repo->markStarred(msg.id.value(), true);
    EXPECT_TRUE(m_repo->findByID(msg.id.value())->is_starred);
    m_repo->markStarred(msg.id.value(), false);
    EXPECT_FALSE(m_repo->findByID(msg.id.value())->is_starred);
}

TEST_F(MessageRepositoryTest, MarkImportant_Toggles)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    m_repo->markImportant(msg.id.value(), true);
    EXPECT_TRUE(m_repo->findByID(msg.id.value())->is_important);
    m_repo->markImportant(msg.id.value(), false);
    EXPECT_FALSE(m_repo->findByID(msg.id.value())->is_important);
}

TEST_F(MessageRepositoryTest, MoveToFolder_Updates)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    EXPECT_TRUE(m_repo->moveToFolder(msg.id.value(), 2));
    EXPECT_EQ(m_repo->findByID(msg.id.value())->folder_id, 2);
}

TEST_F(MessageRepositoryTest, MoveToFolder_AcceptsNullopt)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    EXPECT_TRUE(m_repo->moveToFolder(msg.id.value(), std::nullopt));
    EXPECT_FALSE(m_repo->findByID(msg.id.value())->folder_id.has_value());
}

TEST_F(MessageRepositoryTest, MoveToFolder_RejectsUnknown)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    EXPECT_FALSE(m_repo->moveToFolder(msg.id.value(), 99999));
}

TEST_F(MessageRepositoryTest, MoveToTrash_SetsDeleted)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    EXPECT_TRUE(m_repo->moveToTrash(msg.id.value()));
    EXPECT_EQ(m_repo->findByID(msg.id.value())->status, MessageStatus::Deleted);
}

TEST_F(MessageRepositoryTest, MoveToTrash_RejectsAlreadyDeleted)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    m_repo->moveToTrash(msg.id.value());
    EXPECT_FALSE(m_repo->moveToTrash(msg.id.value()));
}

TEST_F(MessageRepositoryTest, Restore_RecoversTrashedMessage)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    m_repo->moveToTrash(msg.id.value());
    EXPECT_TRUE(m_repo->restore(msg.id.value()));
    EXPECT_NE(m_repo->findByID(msg.id.value())->status, MessageStatus::Deleted);
}

TEST_F(MessageRepositoryTest, Restore_RejectsNonDeleted)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    EXPECT_FALSE(m_repo->restore(msg.id.value()));
}

TEST_F(MessageRepositoryTest, HardDelete_RemovesMessage)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    m_repo->moveToTrash(msg.id.value());
    int64_t id = msg.id.value();
    EXPECT_TRUE(m_repo->hardDelete(id));
    EXPECT_FALSE(m_repo->findByID(id).has_value());
}

TEST_F(MessageRepositoryTest, HardDelete_RejectsNonDeleted)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    EXPECT_FALSE(m_repo->hardDelete(msg.id.value()));
}

TEST_F(MessageRepositoryTest, FindByUser_ReturnsOnlyThatUser)
{
    m_repo->saveDraft(makeDraft("M1"));
    m_repo->saveDraft(makeDraft("M2"));
    EXPECT_EQ(m_repo->findByUser(1).size(), 2u);
}

TEST_F(MessageRepositoryTest, FindUnseen_ReturnsOnlyUnseen)
{
    auto m1 = makeDraft(); m_repo->setDelivered(m1);
    auto m2 = makeDraft(); m_repo->saveDraft(m2);
    EXPECT_EQ(m_repo->findUnseen(1).size(), 1u);
}

TEST_F(MessageRepositoryTest, FindStarred_ReturnsOnlyStarred)
{
    auto m1 = makeDraft(); m_repo->saveDraft(m1);
    auto m2 = makeDraft(); m_repo->saveDraft(m2);
    m_repo->markStarred(m1.id.value(), true);
    EXPECT_EQ(m_repo->findStarred(1).size(), 1u);
}

TEST_F(MessageRepositoryTest, FindImportant_ReturnsOnlyImportant)
{
    auto m1 = makeDraft(); m_repo->saveDraft(m1);
    auto m2 = makeDraft(); m_repo->saveDraft(m2);
    m_repo->markImportant(m1.id.value(), true);
    EXPECT_EQ(m_repo->findImportant(1).size(), 1u);
}

TEST_F(MessageRepositoryTest, Search_FindsBySubject)
{
    auto msg = makeDraft("Invoice Q3"); m_repo->saveDraft(msg);
    EXPECT_EQ(m_repo->search(1, "Invoice").size(), 1u);
}

TEST_F(MessageRepositoryTest, CreateFolder_SetsID)
{
    Folder f; f.user_id = 1; f.name = "Work";
    EXPECT_TRUE(m_repo->createFolder(f));
    EXPECT_TRUE(f.id.has_value());
}

TEST_F(MessageRepositoryTest, CreateFolder_RejectsDuplicate)
{
    Folder f1; f1.user_id = 1; f1.name = "Work";
    Folder f2; f2.user_id = 1; f2.name = "Work";
    m_repo->createFolder(f1);
    EXPECT_FALSE(m_repo->createFolder(f2));
}

TEST_F(MessageRepositoryTest, RenameFolder_Updates)
{
    Folder f; f.user_id = 1; f.name = "Old"; m_repo->createFolder(f);
    EXPECT_TRUE(m_repo->renameFolder(f.id.value(), "New"));
    EXPECT_EQ(m_repo->findFolderByID(f.id.value())->name, "New");
}

TEST_F(MessageRepositoryTest, RenameFolder_RejectsTakenName)
{
    Folder f1; f1.user_id = 1; f1.name = "Taken"; m_repo->createFolder(f1);
    Folder f2; f2.user_id = 1; f2.name = "Other"; m_repo->createFolder(f2);
    EXPECT_FALSE(m_repo->renameFolder(f2.id.value(), "Taken"));
}

TEST_F(MessageRepositoryTest, DeleteFolder_RemovesIt)
{
    Folder f; f.user_id = 1; f.name = "Temp"; m_repo->createFolder(f);
    int64_t id = f.id.value();
    EXPECT_TRUE(m_repo->deleteFolder(id));
    EXPECT_FALSE(m_repo->findFolderByID(id).has_value());
}

TEST_F(MessageRepositoryTest, DeleteFolder_RejectsNonExistent)
{
    EXPECT_FALSE(m_repo->deleteFolder(99999));
}

TEST_F(MessageRepositoryTest, DeleteFolder_SetsMessageFolderNull)
{
    Folder f; f.user_id = 1; f.name = "Temp"; m_repo->createFolder(f);
    auto msg = makeDraft(); msg.folder_id = f.id.value(); m_repo->saveDraft(msg);
    m_repo->deleteFolder(f.id.value());
    EXPECT_FALSE(m_repo->findByID(msg.id.value())->folder_id.has_value());
}

TEST_F(MessageRepositoryTest, AddAttachment_SetsID)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    Attachment a; a.message_id = msg.id.value(); a.file_name = "f.pdf"; a.storage_path = "/f.pdf";
    EXPECT_TRUE(m_repo->addAttachment(a));
    EXPECT_TRUE(a.id.has_value());
}

TEST_F(MessageRepositoryTest, AddAttachment_RejectsUnknownMessage)
{
    Attachment a; a.message_id = 99999; a.file_name = "f.pdf"; a.storage_path = "/f.pdf";
    EXPECT_FALSE(m_repo->addAttachment(a));
}

TEST_F(MessageRepositoryTest, AddAttachment_RejectsEmptyFileName)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    Attachment a; a.message_id = msg.id.value(); a.file_name = ""; a.storage_path = "/f.pdf";
    EXPECT_FALSE(m_repo->addAttachment(a));
}

TEST_F(MessageRepositoryTest, RemoveAttachment_RemovesIt)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    Attachment a; a.message_id = msg.id.value(); a.file_name = "f.pdf"; a.storage_path = "/f.pdf";
    m_repo->addAttachment(a);
    int64_t id = a.id.value();
    EXPECT_TRUE(m_repo->removeAttachment(id));
    EXPECT_FALSE(m_repo->findAttachmentByID(id).has_value());
}

TEST_F(MessageRepositoryTest, RemoveAttachment_RejectsNonExistent)
{
    EXPECT_FALSE(m_repo->removeAttachment(99999));
}

TEST_F(MessageRepositoryTest, AddRecipient_SetsID)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    Recipient r; r.message_id = msg.id.value(); r.address = "alice@example.com";
    EXPECT_TRUE(m_repo->addRecipient(r));
    EXPECT_TRUE(r.id.has_value());
}

TEST_F(MessageRepositoryTest, AddRecipient_RejectsUnknownMessage)
{
    Recipient r; r.message_id = 99999; r.address = "alice@example.com";
    EXPECT_FALSE(m_repo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, AddRecipient_RejectsEmptyAddress)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    Recipient r; r.message_id = msg.id.value(); r.address = "";
    EXPECT_FALSE(m_repo->addRecipient(r));
}

TEST_F(MessageRepositoryTest, AddRecipient_AllowsMultiple)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    Recipient r1; r1.message_id = msg.id.value(); r1.address = "a@example.com";
    Recipient r2; r2.message_id = msg.id.value(); r2.address = "b@example.com";
    m_repo->addRecipient(r1); m_repo->addRecipient(r2);
    EXPECT_EQ(m_repo->findRecipientsByMessage(msg.id.value()).size(), 2u);
}

TEST_F(MessageRepositoryTest, RemoveRecipient_RemovesIt)
{
    auto msg = makeDraft(); m_repo->saveDraft(msg);
    Recipient r; r.message_id = msg.id.value(); r.address = "alice@example.com";
    m_repo->addRecipient(r);
    int64_t id = r.id.value();
    EXPECT_TRUE(m_repo->removeRecipient(id));
    EXPECT_FALSE(m_repo->findRecipientByID(id).has_value());
}

TEST_F(MessageRepositoryTest, RemoveRecipient_RejectsNonExistent)
{
    EXPECT_FALSE(m_repo->removeRecipient(99999));
}