#pragma once

#include <gtest/gtest.h>
#include <sqlite3.h>

#include "../DAL/UserDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/MessageDAL.h"
#include "../DAL/RecipientDAL.h"
#include "../Repository/UserRepository.h"
#include "../Repository/MessageRepository.h"

static const char* TEST_SCHEMA = R"sql(
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS folders (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id       INTEGER NOT NULL,
    name          TEXT NOT NULL,
    next_uid      INTEGER NOT NULL DEFAULT 1,
    is_subscribed INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS messages (
    id                INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id           INTEGER NOT NULL,
    folder_id         INTEGER NOT NULL,
    uid               INTEGER NOT NULL,
    raw_file_path     TEXT NOT NULL,
    size_bytes        INTEGER NOT NULL DEFAULT 0,
    mime_structure    TEXT,
    message_id_header TEXT,
    in_reply_to       TEXT,
    references_header TEXT,
    from_address      TEXT NOT NULL,
    sender_address    TEXT,
    subject           TEXT,
    is_seen           INTEGER NOT NULL DEFAULT 0,
    is_deleted        INTEGER NOT NULL DEFAULT 0,
    is_draft          INTEGER NOT NULL DEFAULT 0,
    is_answered       INTEGER NOT NULL DEFAULT 0,
    is_flagged        INTEGER NOT NULL DEFAULT 0,
    is_recent         INTEGER NOT NULL DEFAULT 1,
    internal_date     TEXT DEFAULT (datetime('now')),
    date_header       TEXT,
    FOREIGN KEY (user_id)   REFERENCES users(id)   ON DELETE CASCADE,
    FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE
);

CREATE UNIQUE INDEX IF NOT EXISTS idx_messages_folder_uid ON messages(folder_id, uid);

CREATE TABLE IF NOT EXISTS recipients (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    address    TEXT NOT NULL,
    type       TEXT NOT NULL CHECK(type IN ('to', 'cc', 'bcc', 'reply-to')),
    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE
);
)sql";

struct DBFixture : public ::testing::Test
{
    sqlite3* db = nullptr;

    void SetUp() override
    {
        sqlite3_open(":memory:", &db);
        sqlite3_exec(db, TEST_SCHEMA, nullptr, nullptr, nullptr);
    }

    void TearDown() override
    {
        sqlite3_close(db);
    }
};

inline User makeUser(const std::string& username = "alice",
                     const std::string& password  = "hash_abc")
{
    User u;
    u.username      = username;
    u.password_hash = password;
    return u;
}

inline Folder makeFolder(int64_t user_id, const std::string& name = "INBOX")
{
    Folder f;
    f.user_id = user_id;
    f.name    = name;
    return f;
}

inline Message makeMessage(int64_t user_id,
                            const std::string& from = "sender@example.com")
{
    Message m;
    m.user_id       = user_id;
    m.folder_id     = 0;
    m.uid           = 0;
    m.raw_file_path = "/mail/test.eml";
    m.size_bytes    = 1024;
    m.from_address  = from;
    m.subject       = "Hello";
    return m;
}