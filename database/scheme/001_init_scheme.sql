PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS folders (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name    TEXT NOT NULL,

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS messages (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id    INTEGER NOT NULL,
    folder_id  INTEGER,

    subject    TEXT,
    body       TEXT,

    status     TEXT NOT NULL CHECK(status IN ('draft', 'sent', 'received', 'deleted')),
    is_seen    INTEGER NOT NULL DEFAULT 0,     -- 0 = false, 1 = true
    created_at TEXT DEFAULT (datetime('now')), -- ISO 8601 format

    FOREIGN KEY (user_id)   REFERENCES users(id)   ON DELETE CASCADE,
    FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS recipients (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    address    TEXT NOT NULL,

    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE
);

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

CREATE TABLE IF NOT EXISTS tags (
    id      INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    name    TEXT NOT NULL,

    UNIQUE (user_id, name),

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

CREATE TABLE IF NOT EXISTS message_tags (
    user_id    INTEGER NOT NULL,
    message_id INTEGER NOT NULL,
    tag_id     INTEGER NOT NULL,

    PRIMARY KEY (user_id, message_id, tag_id),

    FOREIGN KEY (user_id)    REFERENCES users(id)    ON DELETE CASCADE,
    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE,
    FOREIGN KEY (tag_id)     REFERENCES tags(id)     ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_messages_user_id    ON messages(user_id);
CREATE INDEX IF NOT EXISTS idx_messages_folder_id  ON messages(folder_id);
CREATE INDEX IF NOT EXISTS idx_messages_status     ON messages(status);
CREATE INDEX IF NOT EXISTS idx_recipients_message  ON recipients(message_id);
CREATE INDEX IF NOT EXISTS idx_attachments_message ON attachments(message_id);
CREATE INDEX IF NOT EXISTS idx_folders_user_id     ON folders(user_id);
CREATE INDEX IF NOT EXISTS idx_tags_user_id        ON tags(user_id);