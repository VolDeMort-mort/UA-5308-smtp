#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip>

#include "DatabaseManager.h"
#include "DAL/MessageDAL.h"
#include "Repository/MessageRepository.h"
#include "DAL/AttachmentDAL.h"

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void printHeader(const std::string& title)
{
    std::cout << "\n========================================\n";
    std::cout << " TEST: " << title << "\n";
    std::cout << "========================================\n";
}

void printMessage(const Message& msg)
{
    std::cout << "  [ID: " << (msg.id.has_value() ? std::to_string(msg.id.value()) : "N/A") << "] "
              << "Subject: '" << msg.subject << "' | "
              << "Status: " << Message::statusToString(msg.status) << " | "
              << "Seen: "      << (msg.is_seen      ? "Yes" : "No") << " | "
              << "Starred: "   << (msg.is_starred   ? "Yes" : "No") << " | "
              << "Important: " << (msg.is_important ? "Yes" : "No") << "\n";
}

void printAttachment(const Attachment& a)
{
    std::cout << "  [ID: " << (a.id.has_value() ? std::to_string(a.id.value()) : "N/A") << "] "
              << "File: '"  << a.file_name << "' | "
              << "MIME: "   << (a.mime_type.has_value() ? a.mime_type.value() : "NULL") << " | "
              << "Size: "   << (a.file_size.has_value() ? std::to_string(a.file_size.value()) : "NULL") << " bytes | "
              << "Path: '"  << a.storage_path << "'\n";
}

bool setupPrerequisites(sqlite3* db)
{
    char* errMsg = nullptr;

    const char* sqlUser    = "INSERT OR IGNORE INTO users (id, username, password_hash) VALUES (1, 'tester', 'hash123');";
    const char* sqlFolder1 = "INSERT OR IGNORE INTO folders (id, user_id, name) VALUES (1, 1, 'Inbox');";
    const char* sqlFolder2 = "INSERT OR IGNORE INTO folders (id, user_id, name) VALUES (2, 1, 'Archive');";

    if (sqlite3_exec(db, sqlUser,    nullptr, nullptr, &errMsg) != SQLITE_OK) { std::cerr << "Setup User failed: "    << errMsg << "\n"; return false; }
    if (sqlite3_exec(db, sqlFolder1, nullptr, nullptr, &errMsg) != SQLITE_OK) { std::cerr << "Setup Folder1 failed: " << errMsg << "\n"; return false; }
    if (sqlite3_exec(db, sqlFolder2, nullptr, nullptr, &errMsg) != SQLITE_OK) { std::cerr << "Setup Folder2 failed: " << errMsg << "\n"; return false; }

    return true;
}

// ---------------------------------------------------------------------------
// MessageDAL Tests
// ---------------------------------------------------------------------------

void testMessageDAL(MessageDAL& dal, int64_t userId, int64_t folderId)
{
    std::cout << "\n\n####################################\n";
    std::cout << "##  MessageDAL Tests              ##\n";
    std::cout << "####################################\n";

    // --- INSERT ---
    printHeader("DAL: INSERT");

    Message msg1;
    msg1.user_id   = userId;
    msg1.folder_id = folderId;
    msg1.receiver  = "friend@example.com";
    msg1.subject   = "Hello World";
    msg1.body      = "This is the first test message.";
    msg1.status    = MessageStatus::Draft;

    if (dal.insert(msg1))
        std::cout << "PASS: Message inserted. ID: " << msg1.id.value() << "\n";
    else
        std::cerr << "FAIL: " << dal.getLastError() << "\n";

    Message msg2;
    msg2.user_id   = userId;
    msg2.folder_id = folderId;
    msg2.receiver  = "boss@example.com";
    msg2.subject   = "Urgent Report";
    msg2.body      = "Please find attached.";
    msg2.status    = MessageStatus::Sent;
    dal.insert(msg2);

    // --- FIND BY ID ---
    printHeader("DAL: FIND BY ID");

    auto fetched = dal.findByID(msg1.id.value());
    if (fetched.has_value())
    {
        std::cout << "PASS: Message found.\n";
        printMessage(fetched.value());
        if (fetched->body == msg1.body) std::cout << "PASS: Body matches.\n";
        else                            std::cerr << "FAIL: Body mismatch.\n";
    }
    else std::cerr << "FAIL: Message not found.\n";

    // --- UPDATE ---
    printHeader("DAL: UPDATE");

    msg1.subject = "Hello World (Updated)";
    msg1.body    = "Body changed.";

    if (dal.update(msg1))
    {
        auto check = dal.findByID(msg1.id.value());
        if (check->subject == "Hello World (Updated)") std::cout << "PASS: Subject updated.\n";
        else                                           std::cerr << "FAIL: Subject mismatch.\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    // --- UPDATE STATUS ---
    printHeader("DAL: UPDATE STATUS");

    if (dal.updateStatus(msg1.id.value(), MessageStatus::Received))
    {
        auto check = dal.findByID(msg1.id.value());
        if (check->status == MessageStatus::Received) std::cout << "PASS: Status -> Received.\n";
        else                                          std::cerr << "FAIL: Status mismatch.\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    // --- UPDATE FLAGS & SEEN ---
    printHeader("DAL: UPDATE FLAGS & SEEN");

    dal.updateSeen(msg1.id.value(), true);
    dal.updateFlags(msg1.id.value(), true, true, true);

    auto flagCheck = dal.findByID(msg1.id.value());
    if (flagCheck->is_seen && flagCheck->is_starred && flagCheck->is_important)
        std::cout << "PASS: All flags TRUE.\n";
    else
        std::cerr << "FAIL: Flags not set correctly.\n";

    // --- MOVE TO FOLDER ---
    printHeader("DAL: MOVE TO FOLDER");

    if (dal.moveToFolder(msg1.id.value(), 2))
    {
        auto check = dal.findByID(msg1.id.value());
        if (check->folder_id == 2) std::cout << "PASS: Moved to folder 2.\n";
        else                       std::cerr << "FAIL: folder_id mismatch.\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    // --- FIND BY USER ---
    printHeader("DAL: FIND BY USER");

    auto userMsgs = dal.findByUser(userId);
    std::cout << "Found " << userMsgs.size() << " messages.\n";
    for (const auto& m : userMsgs) printMessage(m);
    if (userMsgs.size() >= 2) std::cout << "PASS: Expected count.\n";
    else                      std::cerr << "FAIL: Count mismatch.\n";

    // --- FIND BY STATUS ---
    printHeader("DAL: FIND BY STATUS");

    auto sentMsgs = dal.findByStatus(userId, MessageStatus::Sent);
    std::cout << "Sent count: " << sentMsgs.size() << " (expected >= 1)\n";
    if (!sentMsgs.empty()) std::cout << "PASS\n";
    else                   std::cerr << "FAIL\n";

    // --- FIND UNSEEN ---
    printHeader("DAL: FIND UNSEEN");

    auto unseen = dal.findUnseen(userId);
    std::cout << "Unseen count: " << unseen.size() << " (msg2 should be unseen)\n";
    if (!unseen.empty()) std::cout << "PASS\n";
    else                 std::cerr << "FAIL\n";

    // --- FIND STARRED ---
    printHeader("DAL: FIND STARRED");

    auto starred = dal.findStarred(userId);
    std::cout << "Starred count: " << starred.size() << " (expected >= 1)\n";
    if (!starred.empty()) std::cout << "PASS\n";
    else                  std::cerr << "FAIL\n";

    // --- FIND IMPORTANT ---
    printHeader("DAL: FIND IMPORTANT");

    auto important = dal.findImportant(userId);
    std::cout << "Important count: " << important.size() << " (expected >= 1)\n";
    if (!important.empty()) std::cout << "PASS\n";
    else                    std::cerr << "FAIL\n";

    // --- SEARCH ---
    printHeader("DAL: SEARCH");

    auto searchResults = dal.search(userId, "Urgent");
    if (!searchResults.empty()) std::cout << "PASS: Found via search 'Urgent'.\n";
    else                        std::cerr << "FAIL: Search returned nothing.\n";

    // --- SOFT DELETE ---
    printHeader("DAL: SOFT DELETE");

    if (dal.softDelete(msg1.id.value()))
    {
        auto check = dal.findByID(msg1.id.value());
        if (check->status == MessageStatus::Deleted) std::cout << "PASS: Status -> Deleted.\n";
        else std::cerr << "FAIL: Status is " << Message::statusToString(check->status) << "\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    // --- HARD DELETE ---
    printHeader("DAL: HARD DELETE");

    if (dal.hardDelete(msg1.id.value()))
    {
        auto check = dal.findByID(msg1.id.value());
        if (!check.has_value()) std::cout << "PASS: Message permanently removed.\n";
        else                    std::cerr << "FAIL: Message still exists.\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    // --- ERROR HANDLING ---
    printHeader("DAL: ERROR HANDLING (FK violation)");

    Message badMsg;
    badMsg.user_id  = 99999;
    badMsg.receiver = "fail@test.com";
    badMsg.status   = MessageStatus::Draft;

    if (!dal.insert(badMsg))
    {
        std::cout << "PASS: Insert correctly rejected.\n";
        std::cout << "Error: " << dal.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected unknown user_id.\n";
}

// ---------------------------------------------------------------------------
// MessageRepository Tests
// ---------------------------------------------------------------------------

void testMessageRepository(MessageRepository& repo, int64_t userId, int64_t folderId)
{
    std::cout << "\n\n####################################\n";
    std::cout << "##  MessageRepository Tests       ##\n";
    std::cout << "####################################\n";

    // --- SAVE DRAFT ---
    printHeader("REPO: SAVE DRAFT");

    Message draft;
    draft.user_id   = userId;
    draft.folder_id = folderId;
    draft.receiver  = "colleague@example.com";
    draft.subject   = "Draft Subject";
    draft.body      = "Draft body content.";
    draft.status    = MessageStatus::Received; // should be overridden to Draft

    if (repo.saveDraft(draft))
    {
        auto check = repo.findByID(draft.id.value());
        if (check->status == MessageStatus::Draft && check->is_seen)
            std::cout << "PASS: Draft saved. Status forced to Draft, is_seen = true.\n";
        else
            std::cerr << "FAIL: Draft status or is_seen incorrect.\n";
        printMessage(check.value());
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- EDIT DRAFT ---
    printHeader("REPO: EDIT DRAFT");

    draft.subject = "Draft Subject (Edited)";
    draft.body    = "Edited body.";

    if (repo.editDraft(draft))
    {
        auto check = repo.findByID(draft.id.value());
        if (check->subject == "Draft Subject (Edited)") std::cout << "PASS: Draft edited.\n";
        else                                            std::cerr << "FAIL: Edit not persisted.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- SEND (from draft) ---
    printHeader("REPO: SEND (from draft)");

    if (repo.send(draft))
    {
        auto check = repo.findByID(draft.id.value());
        if (check->status == MessageStatus::Sent) std::cout << "PASS: Draft -> Sent.\n";
        else                                      std::cerr << "FAIL: Status mismatch.\n";
        printMessage(check.value());
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- SEND already-sent (should fail) ---
    printHeader("REPO: SEND already-sent (expect failure)");

    if (!repo.send(draft))
    {
        std::cout << "PASS: Correctly rejected re-send.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected re-sending a Sent message.\n";

    // --- DELIVER ---
    printHeader("REPO: DELIVER");

    Message inbound;
    inbound.user_id   = userId;
    inbound.folder_id = folderId;
    inbound.receiver  = "tester@example.com";
    inbound.subject   = "You've got mail";
    inbound.body      = "Inbound message body.";
    inbound.status    = MessageStatus::Sent; // should be overridden to Received

    if (repo.deliver(inbound))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->status == MessageStatus::Received && !check->is_seen)
            std::cout << "PASS: Delivered. Status = Received, is_seen = false.\n";
        else
            std::cerr << "FAIL: Delivered message has wrong status or is_seen.\n";
        printMessage(check.value());
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- MARK SEEN ---
    printHeader("REPO: MARK SEEN");

    if (repo.markSeen(inbound.id.value(), true))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->is_seen) std::cout << "PASS: Marked as seen.\n";
        else                std::cerr << "FAIL: is_seen still false.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- MARK STARRED ---
    printHeader("REPO: MARK STARRED");

    if (repo.markStarred(inbound.id.value(), true))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->is_starred) std::cout << "PASS: Marked as starred.\n";
        else                   std::cerr << "FAIL: is_starred still false.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- MARK IMPORTANT ---
    printHeader("REPO: MARK IMPORTANT");

    if (repo.markImportant(inbound.id.value(), true))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->is_important) std::cout << "PASS: Marked as important.\n";
        else                     std::cerr << "FAIL: is_important still false.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- MOVE TO FOLDER ---
    printHeader("REPO: MOVE TO FOLDER");

    if (repo.moveToFolder(inbound.id.value(), 2))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->folder_id == 2) std::cout << "PASS: Moved to folder 2.\n";
        else                       std::cerr << "FAIL: folder_id mismatch.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- MOVE TO TRASH ---
    printHeader("REPO: MOVE TO TRASH");

    if (repo.moveToTrash(inbound.id.value()))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->status == MessageStatus::Deleted) std::cout << "PASS: Moved to trash.\n";
        else                                         std::cerr << "FAIL: Status mismatch.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- MOVE TO TRASH again (should fail) ---
    printHeader("REPO: MOVE TO TRASH again (expect failure)");

    if (!repo.moveToTrash(inbound.id.value()))
    {
        std::cout << "PASS: Correctly rejected double-trash.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected already-deleted message.\n";

    // --- RESTORE ---
    printHeader("REPO: RESTORE");

    if (repo.restore(inbound.id.value()))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->status == MessageStatus::Received || check->status == MessageStatus::Sent)
        {
            std::cout << "PASS: Restored to "
                      << Message::statusToString(check->status) << ".\n";
        }
        else std::cerr << "FAIL: Unexpected status after restore.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- PURGE (should fail - not deleted) ---
    printHeader("REPO: PURGE non-deleted message (expect failure)");

    if (!repo.hardDelete(inbound.id.value()))
    {
        std::cout << "PASS: Correctly rejected purge on non-deleted message.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected purge.\n";

    // --- PURGE (correct flow: trash then purge) ---
    printHeader("REPO: PURGE (trash first, then purge)");

    repo.moveToTrash(inbound.id.value());
    if (repo.hardDelete(inbound.id.value()))
    {
        auto check = repo.findByID(inbound.id.value());
        if (!check.has_value()) std::cout << "PASS: Message permanently removed.\n";
        else                    std::cerr << "FAIL: Message still exists.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    // --- QUERIES ---
    printHeader("REPO: QUERIES (findByUser, findStarred, findImportant, search)");

    auto all = repo.findByUser(userId);
    std::cout << "findByUser: " << all.size() << " messages.\n";
    for (const auto& m : all) printMessage(m);

    auto starred   = repo.findStarred(userId);
    auto important = repo.findImportant(userId);
    auto results   = repo.search(userId, "Draft");

    std::cout << "findStarred:   " << starred.size()   << "\n";
    std::cout << "findImportant: " << important.size() << "\n";
    std::cout << "search('Draft'): " << results.size() << "\n";
}

// ---------------------------------------------------------------------------
// AttachmentDAL Tests
// ---------------------------------------------------------------------------

void testAttachmentDAL(AttachmentDAL& aDal, int64_t messageId)
{
    std::cout << "\n\n####################################\n";
    std::cout << "##  AttachmentDAL Tests           ##\n";
    std::cout << "####################################\n";

    // --- INSERT ---
    printHeader("ATTACHMENT: INSERT");

    Attachment att1;
    att1.message_id   = messageId;
    att1.file_name    = "report.pdf";
    att1.mime_type    = "application/pdf";
    att1.file_size    = 204800; // 200 KB
    att1.storage_path = "/files/report.pdf";

    if (aDal.insert(att1))
    {
        std::cout << "PASS: Attachment inserted. ID: " << att1.id.value() << "\n";
        printAttachment(att1);
    }
    else std::cerr << "FAIL: " << aDal.getLastError() << "\n";

    // Insert one with nullable fields
    Attachment att2;
    att2.message_id   = messageId;
    att2.file_name    = "photo.jpg";
    att2.mime_type    = std::nullopt;   // nullable
    att2.file_size    = std::nullopt;   // nullable
    att2.storage_path = "/files/photo.jpg";

    if (aDal.insert(att2))
    {
        std::cout << "PASS: Attachment with null fields inserted. ID: " << att2.id.value() << "\n";
        printAttachment(att2);
    }
    else std::cerr << "FAIL: " << aDal.getLastError() << "\n";

    // --- FIND BY ID ---
    printHeader("ATTACHMENT: FIND BY ID");

    auto fetched = aDal.findByID(att1.id.value());
    if (fetched.has_value())
    {
        std::cout << "PASS: Attachment found.\n";
        printAttachment(fetched.value());
        if (fetched->file_name    == att1.file_name)    std::cout << "PASS: file_name matches.\n";
        if (fetched->storage_path == att1.storage_path) std::cout << "PASS: storage_path matches.\n";
        if (fetched->mime_type    == att1.mime_type)    std::cout << "PASS: mime_type matches.\n";
        if (fetched->file_size    == att1.file_size)    std::cout << "PASS: file_size matches.\n";
    }
    else std::cerr << "FAIL: Attachment not found.\n";

    // Verify nullable fields on att2
    auto fetched2 = aDal.findByID(att2.id.value());
    if (fetched2.has_value())
    {
        if (!fetched2->mime_type.has_value() && !fetched2->file_size.has_value())
            std::cout << "PASS: Nullable fields correctly read as nullopt.\n";
        else
            std::cerr << "FAIL: Nullable fields not null.\n";
    }

    // --- FIND BY MESSAGE ---
    printHeader("ATTACHMENT: FIND BY MESSAGE");

    auto byMsg = aDal.findByMessage(messageId);
    std::cout << "Found " << byMsg.size() << " attachments for message " << messageId << ".\n";
    for (const auto& a : byMsg) printAttachment(a);
    if (byMsg.size() >= 2) std::cout << "PASS: Expected count.\n";
    else                   std::cerr << "FAIL: Count mismatch.\n";

    // --- UPDATE ---
    printHeader("ATTACHMENT: UPDATE");

    att1.file_name    = "report_v2.pdf";
    att1.file_size    = 409600; // 400 KB
    att1.mime_type    = "application/pdf";

    if (aDal.update(att1))
    {
        auto check = aDal.findByID(att1.id.value());
        if (check->file_name == "report_v2.pdf" && check->file_size == 409600)
            std::cout << "PASS: Attachment updated.\n";
        else
            std::cerr << "FAIL: Update not persisted.\n";
        printAttachment(check.value());
    }
    else std::cerr << "FAIL: " << aDal.getLastError() << "\n";

    // --- HARD DELETE ---
    printHeader("ATTACHMENT: HARD DELETE");

    if (aDal.hardDelete(att1.id.value()))
    {
        auto check = aDal.findByID(att1.id.value());
        if (!check.has_value()) std::cout << "PASS: Attachment permanently removed.\n";
        else                    std::cerr << "FAIL: Attachment still exists.\n";
    }
    else std::cerr << "FAIL: " << aDal.getLastError() << "\n";

    // --- ERROR HANDLING ---
    printHeader("ATTACHMENT: ERROR HANDLING (invalid message_id)");

    Attachment badAtt;
    badAtt.message_id   = 99999;  // non-existent
    badAtt.file_name    = "ghost.txt";
    badAtt.storage_path = "/files/ghost.txt";

    if (!aDal.insert(badAtt))
    {
        std::cout << "PASS: Insert correctly rejected (FK violation).\n";
        std::cout << "Error: " << aDal.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected unknown message_id.\n";
}

// ---------------------------------------------------------------------------
// Main
// ---------------------------------------------------------------------------

int main()
{
    const std::string dbPath  = "scheme/smtp.db";
    const std::string sqlPath = "scheme/001_init_scheme.sql";

    DataBaseManager dbManager(dbPath, sqlPath);
    if (!dbManager.isConnected())
    {
        std::cerr << "CRITICAL: Could not connect to DB.\n";
        return -1;
    }

    if (!setupPrerequisites(dbManager.getDB()))
        return -1;

    const int64_t userId   = 1;
    const int64_t folderId = 1;

    // --- DAL tests ---
    MessageDAL dal(dbManager.getDB());
    testMessageDAL(dal, userId, folderId);

    // --- Repository tests ---
    MessageRepository repo(dbManager);
    testMessageRepository(repo, userId, folderId);

    // --- AttachmentDAL tests ---
    // We need a real message_id to attach to — grab the first surviving message
    auto msgs = dal.findByUser(userId);
    if (msgs.empty())
    {
        std::cerr << "\nSkipping AttachmentDAL tests: no messages found to attach to.\n";
    }
    else
    {
        AttachmentDAL aDal(dbManager.getDB());
        testAttachmentDAL(aDal, msgs.front().id.value());
    }

    std::cout << "\n=== All Tests Completed ===\n";
    return 0;
}