#include <iostream>
#include "DatabaseManager.h"

int main() {
    std::cout << "=== SMTP Database Test ===\n\n";

    std::string dbPath  = "scheme/smtp.db";
    std::string sqlPath = "scheme/001_init_scheme.sql";

    DataBaseManager db(dbPath, sqlPath);

    if (!db.isConnected()) {
        std::cerr << "Failed to connect to database. Exiting.\n";
        return 1;
    }

    sqlite3* conn = db.getDB();
    char* errMsg = nullptr;
    int rc;

    // 1. Insert a test user
    std::cout << "\n[TEST] Inserting user...\n";
    rc = sqlite3_exec(conn,
        "INSERT OR IGNORE INTO users (username, password_hash) "
        "VALUES ('alice', 'hashed_password_123');",
        nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Insert user failed: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        long long userId = sqlite3_last_insert_rowid(conn);
        std::cout << "User inserted with id: " << userId << "\n";
    }

    // 2. Insert a folder
    std::cout << "\n[TEST] Inserting folder...\n";
    rc = sqlite3_exec(conn,
        "INSERT OR IGNORE INTO folders (user_id, name) "
        "VALUES (1, 'Inbox');",
        nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Insert folder failed: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        long long folderId = sqlite3_last_insert_rowid(conn);
        std::cout << "Folder inserted with id: " << folderId << "\n";
    }

    // 3. Insert a test message
    std::cout << "\n[TEST] Inserting message...\n";
    rc = sqlite3_exec(conn,
        "INSERT OR IGNORE INTO messages (user_id, folder_id, subject, body, status, is_seen) "
        "VALUES (1, 1, 'Hello!', 'This is a test email body.', 'received', 0);",
        nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Insert message failed: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        long long msgId = sqlite3_last_insert_rowid(conn);
        std::cout << "Message inserted with id: " << msgId << "\n";
    }

    // 4. Query messages
    std::cout << "\n[TEST] Querying messages...\n";

    auto callback = [](void*, int colCount, char** colValues, char** colNames) -> int {
        for (int i = 0; i < colCount; i++) {
            std::cout << colNames[i] << ": " << (colValues[i] ? colValues[i] : "NULL") << "\n";
        }
        std::cout << "---\n";
        return 0;
    };

    rc = sqlite3_exec(conn,
        "SELECT id, subject, body, status, is_seen, created_at FROM messages;",
        callback, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cerr << "Query failed: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }

    // 5. Test FK enforcement
    std::cout << "\n[TEST] Testing foreign key enforcement...\n";
    rc = sqlite3_exec(conn,
        "INSERT INTO messages (user_id, folder_id, subject, status) "
        "VALUES (9999, NULL, 'Should fail', 'received');",
        nullptr, nullptr, &errMsg);

    if (rc != SQLITE_OK) {
        std::cout << "FK enforcement works! Error caught: " << errMsg << "\n";
        sqlite3_free(errMsg);
    } else {
        std::cerr << "WARNING: FK enforcement did NOT work.\n";
    }

    std::cout << "\n=== All tests done ===\n";
    return 0;
}