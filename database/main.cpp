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

void printFolder(const Folder& f)
{
    std::cout << "  [ID: " << (f.id.has_value() ? std::to_string(f.id.value()) : "N/A") << "] "
              << "Name: '" << f.name << "' | "
              << "UserID: " << f.user_id << "\n";
}

void printRecipient(const Recipient& r)
{
    std::cout << "  [ID: " << (r.id.has_value() ? std::to_string(r.id.value()) : "N/A") << "] "
              << "Address: '" << r.address << "' | "
              << "MessageID: " << r.message_id << "\n";
}

bool setupPrerequisites(sqlite3* db)
{
    char* errMsg = nullptr;

    const char* sqlClean   = "DELETE FROM messages; DELETE FROM attachments; DELETE FROM recipients; DELETE FROM folders; DELETE FROM users; DELETE FROM sqlite_sequence;";
    const char* sqlUser    = "INSERT OR IGNORE INTO users (id, username, password_hash) VALUES (1, 'tester', 'hash123');";
    const char* sqlFolder1 = "INSERT OR IGNORE INTO folders (id, user_id, name) VALUES (1, 1, 'Inbox');";
    const char* sqlFolder2 = "INSERT OR IGNORE INTO folders (id, user_id, name) VALUES (2, 1, 'Archive');";

    if (sqlite3_exec(db, sqlClean,   nullptr, nullptr, &errMsg) != SQLITE_OK) { std::cerr << "Cleanup failed: "       << errMsg << "\n"; return false; }
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

    printHeader("DAL: UPDATE STATUS");

    if (dal.updateStatus(msg1.id.value(), MessageStatus::Received))
    {
        auto check = dal.findByID(msg1.id.value());
        if (check->status == MessageStatus::Received) std::cout << "PASS: Status -> Received.\n";
        else                                          std::cerr << "FAIL: Status mismatch.\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    printHeader("DAL: UPDATE FLAGS & SEEN");

    dal.updateSeen(msg1.id.value(), true);
    dal.updateFlags(msg1.id.value(), true, true, true);

    auto flagCheck = dal.findByID(msg1.id.value());
    if (flagCheck->is_seen && flagCheck->is_starred && flagCheck->is_important)
        std::cout << "PASS: All flags TRUE.\n";
    else
        std::cerr << "FAIL: Flags not set correctly.\n";

    printHeader("DAL: MOVE TO FOLDER");

    if (dal.moveToFolder(msg1.id.value(), 2))
    {
        auto check = dal.findByID(msg1.id.value());
        if (check->folder_id == 2) std::cout << "PASS: Moved to folder 2.\n";
        else                       std::cerr << "FAIL: folder_id mismatch.\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    printHeader("DAL: FIND BY USER");

    auto userMsgs = dal.findByUser(userId);
    std::cout << "Found " << userMsgs.size() << " messages.\n";
    for (const auto& m : userMsgs) printMessage(m);
    if (userMsgs.size() >= 2) std::cout << "PASS: Expected count.\n";
    else                      std::cerr << "FAIL: Count mismatch.\n";

    printHeader("DAL: FIND BY STATUS");

    auto sentMsgs = dal.findByStatus(userId, MessageStatus::Sent);
    std::cout << "Sent count: " << sentMsgs.size() << " (expected >= 1)\n";
    if (!sentMsgs.empty()) std::cout << "PASS\n";
    else                   std::cerr << "FAIL\n";

    printHeader("DAL: FIND UNSEEN");

    auto unseen = dal.findUnseen(userId);
    std::cout << "Unseen count: " << unseen.size() << " (msg2 should be unseen)\n";
    if (!unseen.empty()) std::cout << "PASS\n";
    else                 std::cerr << "FAIL\n";

    printHeader("DAL: FIND STARRED");

    auto starred = dal.findStarred(userId);
    std::cout << "Starred count: " << starred.size() << " (expected >= 1)\n";
    if (!starred.empty()) std::cout << "PASS\n";
    else                  std::cerr << "FAIL\n";

    printHeader("DAL: FIND IMPORTANT");

    auto important = dal.findImportant(userId);
    std::cout << "Important count: " << important.size() << " (expected >= 1)\n";
    if (!important.empty()) std::cout << "PASS\n";
    else                    std::cerr << "FAIL\n";

    printHeader("DAL: SEARCH");

    auto searchResults = dal.search(userId, "Urgent");
    if (!searchResults.empty()) std::cout << "PASS: Found via search 'Urgent'.\n";
    else                        std::cerr << "FAIL: Search returned nothing.\n";

    printHeader("DAL: SOFT DELETE");

    if (dal.softDelete(msg1.id.value()))
    {
        auto check = dal.findByID(msg1.id.value());
        if (check->status == MessageStatus::Deleted) std::cout << "PASS: Status -> Deleted.\n";
        else std::cerr << "FAIL: Status is " << Message::statusToString(check->status) << "\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

    printHeader("DAL: HARD DELETE");

    if (dal.hardDelete(msg1.id.value()))
    {
        auto check = dal.findByID(msg1.id.value());
        if (!check.has_value()) std::cout << "PASS: Message permanently removed.\n";
        else                    std::cerr << "FAIL: Message still exists.\n";
    }
    else std::cerr << "FAIL: " << dal.getLastError() << "\n";

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

    printHeader("REPO: SAVE DRAFT");

    Message draft;
    draft.user_id   = userId;
    draft.folder_id = folderId;
    draft.receiver  = "colleague@example.com";
    draft.subject   = "Draft Subject";
    draft.body      = "Draft body content.";
    draft.status    = MessageStatus::Received;

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

    printHeader("REPO: SEND (from draft)");

    if (repo.send(draft))
    {
        auto check = repo.findByID(draft.id.value());
        if (check->status == MessageStatus::Sent) std::cout << "PASS: Draft -> Sent.\n";
        else                                      std::cerr << "FAIL: Status mismatch.\n";
        printMessage(check.value());
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: SEND already-sent (expect failure)");

    if (!repo.send(draft))
    {
        std::cout << "PASS: Correctly rejected re-send.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected re-sending a Sent message.\n";

    printHeader("REPO: DELIVER");

    Message inbound;
    inbound.user_id   = userId;
    inbound.folder_id = folderId;
    inbound.receiver  = "tester@example.com";
    inbound.subject   = "You've got mail";
    inbound.body      = "Inbound message body.";
    inbound.status    = MessageStatus::Sent;

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

    printHeader("REPO: MARK SEEN");

    if (repo.markSeen(inbound.id.value(), true))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->is_seen) std::cout << "PASS: Marked as seen.\n";
        else                std::cerr << "FAIL: is_seen still false.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: MARK STARRED");

    if (repo.markStarred(inbound.id.value(), true))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->is_starred) std::cout << "PASS: Marked as starred.\n";
        else                   std::cerr << "FAIL: is_starred still false.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: MARK IMPORTANT");

    if (repo.markImportant(inbound.id.value(), true))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->is_important) std::cout << "PASS: Marked as important.\n";
        else                     std::cerr << "FAIL: is_important still false.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: MOVE TO FOLDER");

    if (repo.moveToFolder(inbound.id.value(), 2))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->folder_id == 2) std::cout << "PASS: Moved to folder 2.\n";
        else                       std::cerr << "FAIL: folder_id mismatch.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: MOVE TO TRASH");

    if (repo.moveToTrash(inbound.id.value()))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->status == MessageStatus::Deleted) std::cout << "PASS: Moved to trash.\n";
        else                                         std::cerr << "FAIL: Status mismatch.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: MOVE TO TRASH again (expect failure)");

    if (!repo.moveToTrash(inbound.id.value()))
    {
        std::cout << "PASS: Correctly rejected double-trash.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected already-deleted message.\n";

    printHeader("REPO: RESTORE");

    if (repo.restore(inbound.id.value()))
    {
        auto check = repo.findByID(inbound.id.value());
        if (check->status == MessageStatus::Received || check->status == MessageStatus::Sent)
            std::cout << "PASS: Restored to " << Message::statusToString(check->status) << ".\n";
        else
            std::cerr << "FAIL: Unexpected status after restore.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: PURGE non-deleted message (expect failure)");

    if (!repo.hardDelete(inbound.id.value()))
    {
        std::cout << "PASS: Correctly rejected purge on non-deleted message.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected purge.\n";

    printHeader("REPO: PURGE (trash first, then purge)");

    repo.moveToTrash(inbound.id.value());
    if (repo.hardDelete(inbound.id.value()))
    {
        auto check = repo.findByID(inbound.id.value());
        if (!check.has_value()) std::cout << "PASS: Message permanently removed.\n";
        else                    std::cerr << "FAIL: Message still exists.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("REPO: QUERIES (findByUser, findStarred, findImportant, search)");

    auto all = repo.findByUser(userId);
    std::cout << "findByUser: " << all.size() << " messages.\n";
    for (const auto& m : all) printMessage(m);

    auto starred   = repo.findStarred(userId);
    auto important = repo.findImportant(userId);
    auto results   = repo.search(userId, "Draft");

    std::cout << "findStarred:    " << starred.size()   << "\n";
    std::cout << "findImportant:  " << important.size() << "\n";
    std::cout << "search('Draft'): " << results.size()  << "\n";
}

// ---------------------------------------------------------------------------
// Folder Tests (via MessageRepository)
// ---------------------------------------------------------------------------

void testFolders(MessageRepository& repo, int64_t userId)
{
    std::cout << "\n\n####################################\n";
    std::cout << "##  Folder Tests                  ##\n";
    std::cout << "####################################\n";

    printHeader("FOLDER: CREATE");

    Folder f1;
    f1.user_id = userId;
    f1.name    = "Work";

    if (repo.createFolder(f1))
    {
        std::cout << "PASS: Folder created. ID: " << f1.id.value() << "\n";
        printFolder(f1);
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    Folder f2;
    f2.user_id = userId;
    f2.name    = "Personal";
    repo.createFolder(f2);

    printHeader("FOLDER: CREATE duplicate (expect failure)");

    Folder dup;
    dup.user_id = userId;
    dup.name    = "Work";

    if (!repo.createFolder(dup))
    {
        std::cout << "PASS: Correctly rejected duplicate folder name.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected duplicate.\n";

    printHeader("FOLDER: FIND BY ID");

    auto found = repo.findFolderByID(f1.id.value());
    if (found.has_value() && found->name == "Work") std::cout << "PASS: Found by ID.\n";
    else                                            std::cerr << "FAIL: Not found or name mismatch.\n";

    printHeader("FOLDER: FIND BY NAME");

    auto byName = repo.findFolderByName(userId, "Personal");
    if (byName.has_value()) std::cout << "PASS: Found by name.\n";
    else                    std::cerr << "FAIL: Not found.\n";

    printHeader("FOLDER: FIND BY USER");

    auto all = repo.findFoldersByUser(userId);
    std::cout << "Folders for user: " << all.size() << "\n";
    for (const auto& f : all) printFolder(f);
    if (all.size() >= 2) std::cout << "PASS: Expected count.\n";
    else                 std::cerr << "FAIL: Count mismatch.\n";

    printHeader("FOLDER: RENAME");

    if (repo.renameFolder(f1.id.value(), "Work (Renamed)"))
    {
        auto check = repo.findFolderByID(f1.id.value());
        if (check->name == "Work (Renamed)") std::cout << "PASS: Folder renamed.\n";
        else                                 std::cerr << "FAIL: Name not updated.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("FOLDER: RENAME to existing name (expect failure)");

    if (!repo.renameFolder(f1.id.value(), "Personal"))
    {
        std::cout << "PASS: Correctly rejected rename to existing name.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected rename.\n";

    printHeader("FOLDER: DELETE");

    if (repo.deleteFolder(f2.id.value()))
    {
        auto check = repo.findFolderByID(f2.id.value());
        if (!check.has_value()) std::cout << "PASS: Folder deleted.\n";
        else                    std::cerr << "FAIL: Folder still exists.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("FOLDER: DELETE non-existent (expect failure)");

    if (!repo.deleteFolder(99999))
    {
        std::cout << "PASS: Correctly rejected delete of non-existent folder.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected delete.\n";
}

// ---------------------------------------------------------------------------
// Attachment Tests (via MessageRepository)
// ---------------------------------------------------------------------------

void testAttachments(MessageRepository& repo, int64_t messageId)
{
    std::cout << "\n\n####################################\n";
    std::cout << "##  Attachment Tests              ##\n";
    std::cout << "####################################\n";

    printHeader("ATTACHMENT: ADD");

    Attachment att1;
    att1.message_id   = messageId;
    att1.file_name    = "report.pdf";
    att1.mime_type    = "application/pdf";
    att1.file_size    = 204800;
    att1.storage_path = "/files/report.pdf";

    if (repo.addAttachment(att1))
    {
        std::cout << "PASS: Attachment added. ID: " << att1.id.value() << "\n";
        printAttachment(att1);
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    Attachment att2;
    att2.message_id   = messageId;
    att2.file_name    = "photo.jpg";
    att2.mime_type    = std::nullopt;
    att2.file_size    = std::nullopt;
    att2.storage_path = "/files/photo.jpg";
    repo.addAttachment(att2);

    printHeader("ATTACHMENT: ADD with invalid message_id (expect failure)");

    Attachment badAtt;
    badAtt.message_id   = 99999;
    badAtt.file_name    = "ghost.txt";
    badAtt.storage_path = "/files/ghost.txt";

    if (!repo.addAttachment(badAtt))
    {
        std::cout << "PASS: Correctly rejected invalid message_id.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected unknown message_id.\n";

    printHeader("ATTACHMENT: ADD with empty file_name (expect failure)");

    Attachment emptyName;
    emptyName.message_id   = messageId;
    emptyName.file_name    = "";
    emptyName.storage_path = "/files/something.txt";

    if (!repo.addAttachment(emptyName))
    {
        std::cout << "PASS: Correctly rejected empty file_name.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected empty file_name.\n";

    printHeader("ATTACHMENT: FIND BY ID");

    auto fetched = repo.findAttachmentByID(att1.id.value());
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

    printHeader("ATTACHMENT: FIND BY MESSAGE");

    auto byMsg = repo.findAttachmentsByMessage(messageId);
    std::cout << "Found " << byMsg.size() << " attachments.\n";
    for (const auto& a : byMsg) printAttachment(a);
    if (byMsg.size() >= 2) std::cout << "PASS: Expected count.\n";
    else                   std::cerr << "FAIL: Count mismatch.\n";

    printHeader("ATTACHMENT: UPDATE");

    att1.file_name = "report_v2.pdf";
    att1.file_size = 409600;

    if (repo.updateAttachment(att1))
    {
        auto check = repo.findAttachmentByID(att1.id.value());
        if (check->file_name == "report_v2.pdf" && check->file_size == 409600)
            std::cout << "PASS: Attachment updated.\n";
        else
            std::cerr << "FAIL: Update not persisted.\n";
        printAttachment(check.value());
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("ATTACHMENT: REMOVE");

    if (repo.removeAttachment(att1.id.value()))
    {
        auto check = repo.findAttachmentByID(att1.id.value());
        if (!check.has_value()) std::cout << "PASS: Attachment removed.\n";
        else                    std::cerr << "FAIL: Attachment still exists.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("ATTACHMENT: REMOVE non-existent (expect failure)");

    if (!repo.removeAttachment(99999))
    {
        std::cout << "PASS: Correctly rejected remove of non-existent attachment.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected remove.\n";
}

// ---------------------------------------------------------------------------
// Recipient Tests (via MessageRepository)
// ---------------------------------------------------------------------------

void testRecipients(MessageRepository& repo, int64_t messageId)
{
    std::cout << "\n\n####################################\n";
    std::cout << "##  Recipient Tests               ##\n";
    std::cout << "####################################\n";

    printHeader("RECIPIENT: ADD SINGLE");

    Recipient r1;
    r1.message_id = messageId;
    r1.address    = "alice@example.com";

    if (repo.addRecipient(r1))
    {
        std::cout << "PASS: Recipient added. ID: " << r1.id.value() << "\n";
        printRecipient(r1);
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("RECIPIENT: ADD with empty address (expect failure)");

    Recipient empty;
    empty.message_id = messageId;
    empty.address    = "";

    if (!repo.addRecipient(empty))
    {
        std::cout << "PASS: Correctly rejected empty address.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected empty address.\n";

    printHeader("RECIPIENT: ADD with invalid message_id (expect failure)");

    Recipient bad;
    bad.message_id = 99999;
    bad.address    = "ghost@example.com";

    if (!repo.addRecipient(bad))
    {
        std::cout << "PASS: Correctly rejected invalid message_id.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected unknown message_id.\n";

    printHeader("RECIPIENT: ADD MORE SINGLES");

    Recipient r2;
    r2.message_id = messageId;
    r2.address    = "bob@example.com";

    Recipient r3;
    r3.message_id = messageId;
    r3.address    = "carol@example.com";

    repo.addRecipient(r2);
    repo.addRecipient(r3);

    printHeader("RECIPIENT: FIND BY MESSAGE");

    auto byMsg = repo.findRecipientsByMessage(messageId);
    std::cout << "Found " << byMsg.size() << " recipients.\n";
    for (const auto& r : byMsg) printRecipient(r);
    if (byMsg.size() >= 3) std::cout << "PASS: Expected count.\n";
    else                   std::cerr << "FAIL: Count mismatch.\n";

    printHeader("RECIPIENT: FIND BY ID");

    auto found = repo.findRecipientByID(r1.id.value());
    if (found.has_value() && found->address == "alice@example.com") std::cout << "PASS: Found by ID.\n";
    else                                                             std::cerr << "FAIL: Not found or address mismatch.\n";

    printHeader("RECIPIENT: REMOVE SINGLE");

    if (repo.removeRecipient(r1.id.value()))
    {
        auto check = repo.findRecipientByID(r1.id.value());
        if (!check.has_value()) std::cout << "PASS: Recipient removed.\n";
        else                    std::cerr << "FAIL: Recipient still exists.\n";
    }
    else std::cerr << "FAIL: " << repo.getLastError() << "\n";

    printHeader("RECIPIENT: REMOVE remaining one by one");

    repo.removeRecipient(r2.id.value());
    repo.removeRecipient(r3.id.value());

    auto remaining = repo.findRecipientsByMessage(messageId);
    if (remaining.empty()) std::cout << "PASS: All recipients removed individually.\n";
    else                   std::cerr << "FAIL: " << remaining.size() << " recipients still exist.\n";

    printHeader("RECIPIENT: REMOVE non-existent (expect failure)");

    if (!repo.removeRecipient(99999))
    {
        std::cout << "PASS: Correctly rejected remove of non-existent recipient.\n";
        std::cout << "Error: " << repo.getLastError() << "\n";
    }
    else std::cerr << "FAIL: Should have rejected remove.\n";
}

// ---------------------------------------------------------------------------
// AttachmentDAL Tests (direct)
// ---------------------------------------------------------------------------

void testAttachmentDAL(AttachmentDAL& aDal, int64_t messageId)
{
    std::cout << "\n\n####################################\n";
    std::cout << "##  AttachmentDAL Tests           ##\n";
    std::cout << "####################################\n";

    printHeader("ATTACHMENT: INSERT");

    Attachment att1;
    att1.message_id   = messageId;
    att1.file_name    = "report.pdf";
    att1.mime_type    = "application/pdf";
    att1.file_size    = 204800;
    att1.storage_path = "/files/report.pdf";

    if (aDal.insert(att1))
    {
        std::cout << "PASS: Attachment inserted. ID: " << att1.id.value() << "\n";
        printAttachment(att1);
    }
    else std::cerr << "FAIL: " << aDal.getLastError() << "\n";

    Attachment att2;
    att2.message_id   = messageId;
    att2.file_name    = "photo.jpg";
    att2.mime_type    = std::nullopt;
    att2.file_size    = std::nullopt;
    att2.storage_path = "/files/photo.jpg";

    if (aDal.insert(att2))
    {
        std::cout << "PASS: Attachment with null fields inserted. ID: " << att2.id.value() << "\n";
        printAttachment(att2);
    }
    else std::cerr << "FAIL: " << aDal.getLastError() << "\n";

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

    auto fetched2 = aDal.findByID(att2.id.value());
    if (fetched2.has_value())
    {
        if (!fetched2->mime_type.has_value() && !fetched2->file_size.has_value())
            std::cout << "PASS: Nullable fields correctly read as nullopt.\n";
        else
            std::cerr << "FAIL: Nullable fields not null.\n";
    }

    printHeader("ATTACHMENT: FIND BY MESSAGE");

    auto byMsg = aDal.findByMessage(messageId);
    std::cout << "Found " << byMsg.size() << " attachments for message " << messageId << ".\n";
    for (const auto& a : byMsg) printAttachment(a);
    if (byMsg.size() >= 2) std::cout << "PASS: Expected count.\n";
    else                   std::cerr << "FAIL: Count mismatch.\n";

    printHeader("ATTACHMENT: UPDATE");

    att1.file_name = "report_v2.pdf";
    att1.file_size = 409600;
    att1.mime_type = "application/pdf";

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

    printHeader("ATTACHMENT: HARD DELETE");

    if (aDal.hardDelete(att1.id.value()))
    {
        auto check = aDal.findByID(att1.id.value());
        if (!check.has_value()) std::cout << "PASS: Attachment permanently removed.\n";
        else                    std::cerr << "FAIL: Attachment still exists.\n";
    }
    else std::cerr << "FAIL: " << aDal.getLastError() << "\n";

    printHeader("ATTACHMENT: ERROR HANDLING (invalid message_id)");

    Attachment badAtt;
    badAtt.message_id   = 99999;
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

    // --- Folder tests ---
    testFolders(repo, userId);

    // Grab a surviving message for attachment and recipient tests
    auto msgs = repo.findByUser(userId);
    if (msgs.empty())
    {
        std::cerr << "\nSkipping Attachment/Recipient tests: no messages found.\n";
    }
    else
    {
        int64_t messageId = msgs.front().id.value();

        // --- Attachment tests via repository ---
        testAttachments(repo, messageId);

        // --- Recipient tests via repository ---
        testRecipients(repo, messageId);

        // --- AttachmentDAL direct tests ---
        AttachmentDAL aDal(dbManager.getDB());
        testAttachmentDAL(aDal, messageId);
    }

    std::cout << "\n=== All Tests Completed ===\n";
    return 0;
}