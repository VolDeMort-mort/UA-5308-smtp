#include <iostream>
#include <vector>
#include <cassert>
#include <iomanip>

#include "DatabaseManager.h"
#include "MessageDAL.h"

// --- Helper Utilities ---

void printHeader(const std::string& title) {
    std::cout << "\n========================================\n";
    std::cout << " TEST: " << title << "\n";
    std::cout << "========================================\n";
}

void printMessage(const Message& msg) {
    std::cout << "  [ID: " << (msg.id.has_value() ? std::to_string(msg.id.value()) : "N/A") << "] "
              << "Subject: '" << msg.subject << "' | "
              << "Status: " << Message::statusToString(msg.status) << " | "
              << "Seen: " << (msg.is_seen ? "Yes" : "No") << " | "
              << "Starred: " << (msg.is_starred ? "Yes" : "No") << "\n";
}

// Helper to set up prerequisites (User and Folder) since FKs are ON
bool setupPrerequisites(sqlite3* db) {
    char* errMsg = nullptr;
    
    // 1. Create a User (ID 1)
    std::string sqlUser = "INSERT OR IGNORE INTO users (id, username, password_hash) VALUES (1, 'tester', 'hash123');";
    if (sqlite3_exec(db, sqlUser.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Setup User Failed: " << errMsg << "\n";
        return false;
    }

    // 2. Create two Folders (ID 1 and 2)
    std::string sqlFolder1 = "INSERT OR IGNORE INTO folders (id, user_id, name) VALUES (1, 1, 'Inbox');";
    std::string sqlFolder2 = "INSERT OR IGNORE INTO folders (id, user_id, name) VALUES (2, 1, 'Archive');";
    
    if (sqlite3_exec(db, sqlFolder1.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK ||
        sqlite3_exec(db, sqlFolder2.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Setup Folder Failed: " << errMsg << "\n";
        return false;
    }

    return true;
}

int main() {
    // 1. Initialize Database
    std::string dbPath  = "scheme/smtp.db";
    std::string sqlPath = "scheme/001_init_scheme.sql";
    
    // Ensure directory exists or path is correct regarding where you run the executable
    DataBaseManager dbManager(dbPath, sqlPath);

    if (!dbManager.isConnected()) {
        std::cerr << "CRITICAL: Could not connect to DB.\n";
        return -1;
    }

    // 2. Setup Prerequisites (User & Folder)
    if (!setupPrerequisites(dbManager.getDB())) {
        return -1;
    }

    // 3. Initialize DAL
    MessageDAL dal(dbManager.getDB());
    int64_t userId = 1;
    int64_t folderId = 1;

    // ==========================================
    // 1. TEST: Insert
    // ==========================================
    printHeader("INSERT");
    
    Message msg1;
    msg1.user_id = userId;
    msg1.folder_id = folderId;
    msg1.receiver = "friend@example.com";
    msg1.subject = "Hello World";
    msg1.body = "This is the first test message.";
    msg1.status = MessageStatus::Draft;
    
    if (dal.insert(msg1)) {
        std::cout << "PASS: Message inserted. New ID: " << msg1.id.value() << "\n";
    } else {
        std::cerr << "FAIL: Insert failed: " << dal.getLastError() << "\n";
    }

    // Insert a second message for list testing
    Message msg2;
    msg2.user_id = userId;
    msg2.folder_id = folderId;
    msg2.receiver = "boss@example.com";
    msg2.subject = "Urgent Report";
    msg2.body = "Please find attached.";
    msg2.status = MessageStatus::Sent;
    dal.insert(msg2); // ID likely 2

    // ==========================================
    // 2. TEST: Find By ID
    // ==========================================
    printHeader("FIND BY ID");

    auto fetchedMsg = dal.findByID(msg1.id.value());
    if (fetchedMsg.has_value()) {
        std::cout << "PASS: Found message.\n";
        printMessage(fetchedMsg.value());
        
        // Validation
        if (fetchedMsg->body == msg1.body) std::cout << "      Body matches.\n";
        else std::cerr << "      FAIL: Body mismatch.\n";
    } else {
        std::cerr << "FAIL: Message not found.\n";
    }

    // ==========================================
    // 3. TEST: Update (Full Object)
    // ==========================================
    printHeader("UPDATE FULL OBJECT");

    msg1.subject = "Hello World (Updated)";
    msg1.body = "Body changed.";
    
    if (dal.update(msg1)) {
        auto check = dal.findByID(msg1.id.value());
        if (check->subject == "Hello World (Updated)") 
            std::cout << "PASS: Subject updated successfully.\n";
        else 
            std::cerr << "FAIL: Subject did not update.\n";
    } else {
        std::cerr << "FAIL: Update command failed.\n";
    }

    // ==========================================
    // 4. TEST: Update Status
    // ==========================================
    printHeader("UPDATE STATUS");

    // Change Draft -> Received
    if (dal.updateStatus(msg1.id.value(), MessageStatus::Received)) {
        auto check = dal.findByID(msg1.id.value());
        if (check->status == MessageStatus::Received) 
            std::cout << "PASS: Status changed to Received.\n";
        else 
            std::cerr << "FAIL: Status mismatch.\n";
    }

    // ==========================================
    // 5. TEST: Update Flags & Seen
    // ==========================================
    printHeader("UPDATE FLAGS & SEEN");

    // Test updateSeen
    dal.updateSeen(msg1.id.value(), true);
    
    // Test updateFlags (Seen=true, Starred=true, Important=true)
    dal.updateFlags(msg1.id.value(), true, true, true);

    auto flagCheck = dal.findByID(msg1.id.value());
    if (flagCheck->is_seen && flagCheck->is_starred && flagCheck->is_important) {
        std::cout << "PASS: All flags set to TRUE.\n";
    } else {
        std::cerr << "FAIL: Flags not set correctly.\n";
    }

    // ==========================================
    // 6. TEST: Move To Folder
    // ==========================================
    printHeader("MOVE TO FOLDER");

    int64_t archiveFolderId = 2; // Created in setup
    if (dal.moveToFolder(msg1.id.value(), archiveFolderId)) {
        auto folderCheck = dal.findByID(msg1.id.value());
        if (folderCheck->folder_id == archiveFolderId)
            std::cout << "PASS: Moved to folder " << archiveFolderId << "\n";
        else 
            std::cerr << "FAIL: Folder ID not updated.\n";
    }

    // ==========================================
    // 7. TEST: Find By User (List)
    // ==========================================
    printHeader("FIND BY USER");

    std::vector<Message> userMsgs = dal.findByUser(userId);
    std::cout << "Found " << userMsgs.size() << " messages for user " << userId << ".\n";
    for(const auto& m : userMsgs) printMessage(m);

    if (userMsgs.size() >= 2) std::cout << "PASS: Retrieved expected count.\n";
    else std::cerr << "FAIL: Count mismatch.\n";

    // ==========================================
    // 8. TEST: Find By Filters
    // ==========================================
    printHeader("FIND BY FILTERS");

    // Find Starred (We starred msg1 earlier)
    auto starred = dal.findStarred(userId);
    std::cout << "Starred count: " << starred.size() << " (Expected >= 1)\n";

    // Find Important
    auto important = dal.findImportant(userId);
    std::cout << "Important count: " << important.size() << " (Expected >= 1)\n";

    // Find Unseen (msg1 is seen, msg2 is unseen default)
    auto unseen = dal.findUnseen(userId);
    std::cout << "Unseen count: " << unseen.size() << "\n"; 

    // Find By Status (msg2 was 'Sent')
    auto sentMsgs = dal.findByStatus(userId, MessageStatus::Sent);
    std::cout << "Status 'Sent' count: " << sentMsgs.size() << " (Expected >= 1)\n";

    // ==========================================
    // 9. TEST: Search
    // ==========================================
    printHeader("SEARCH");

    // msg2 subject is "Urgent Report"
    auto searchResults = dal.search(userId, "Urgent");
    if (!searchResults.empty()) {
        std::cout << "PASS: Found message via search query 'Urgent'.\n";
    } else {
        std::cerr << "FAIL: Search returned nothing.\n";
    }

    // ==========================================
    // 10. TEST: Soft Delete
    // ==========================================
    printHeader("SOFT DELETE");

    if (dal.softDelete(msg1.id.value())) {
        auto check = dal.findByID(msg1.id.value());
        if (check->status == MessageStatus::Deleted)
            std::cout << "PASS: Status is now 'deleted'.\n";
        else
            std::cerr << "FAIL: Status is " << Message::statusToString(check->status) << "\n";
    }

    // ==========================================
    // 11. TEST: Hard Delete
    // ==========================================
    printHeader("HARD DELETE");

    if (dal.hardDelete(msg1.id.value())) {
        auto check = dal.findByID(msg1.id.value());
        if (!check.has_value()) 
            std::cout << "PASS: Message no longer exists in DB.\n";
        else 
            std::cerr << "FAIL: Message still found.\n";
    }

    // ==========================================
    // 12. TEST: Error Handling (Constraints)
    // ==========================================
    printHeader("ERROR HANDLING");

    Message badMsg;
    badMsg.user_id = 99999; // Non-existbent user
    badMsg.receiver = "fail@test.com";
    badMsg.status = MessageStatus::Draft;

    if (!dal.insert(badMsg)) {
        std::cout << "PASS: Insert rejected correctly.\n";
        std::cout << "Error message: " << dal.getLastError() << "\n";
    } else {
        std::cerr << "FAIL: Insert should have failed (FK violation), but succeeded.\n";
    }

    std::cout << "\n=== All Tests Completed ===\n";
    return 0;
}