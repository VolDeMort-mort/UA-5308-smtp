-- ============================================================
-- 001_init_scheme.sql
-- SMTP Project - Initial Database Schema
-- Database: SQLite
-- ============================================================

PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

-- ------------------------------------------------------------
-- Users
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL
);

-- ------------------------------------------------------------
-- Folders
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS folders (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name    TEXT NOT NULL,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- ------------------------------------------------------------
-- Messages
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS messages (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id      INTEGER NOT NULL,
    folder_id    INTEGER,

    subject      TEXT,
    body         TEXT,

    receiver     TEXT NOT NULL,

    status       TEXT NOT NULL CHECK(status IN ('draft', 'sent', 'received', 'deleted')),
    is_seen      INTEGER NOT NULL DEFAULT 0,  -- 0 = false, 1 = true
    is_starred   INTEGER NOT NULL DEFAULT 0,
    is_important INTEGER NOT NULL DEFAULT 0,

    created_at   TEXT DEFAULT (datetime('now')),

    FOREIGN KEY (user_id)   REFERENCES users(id)   ON DELETE CASCADE,
    FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE SET NULL
);

-- ------------------------------------------------------------
-- Attachments
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS attachments (
    id           INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id   INTEGER NOT NULL,

    file_name    TEXT NOT NULL,
    mime_type    TEXT,
    file_size    INTEGER,
    storage_path TEXT NOT NULL,

    uploaded_at  TEXT DEFAULT (datetime('now')),

    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE
);

-- ------------------------------------------------------------
-- Recipients
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS recipients (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    address    TEXT NOT NULL,

    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE
);

-- ------------------------------------------------------------
-- Indexes
-- ------------------------------------------------------------
CREATE INDEX IF NOT EXISTS idx_messages_user_id    ON messages(user_id);
CREATE INDEX IF NOT EXISTS idx_messages_folder_id  ON messages(folder_id);
CREATE INDEX IF NOT EXISTS idx_messages_status     ON messages(status);
CREATE INDEX IF NOT EXISTS idx_attachments_message ON attachments(message_id);
CREATE INDEX IF NOT EXISTS idx_recipients_message  ON recipients(message_id);
CREATE INDEX IF NOT EXISTS idx_folders_user_id     ON folders(user_id);