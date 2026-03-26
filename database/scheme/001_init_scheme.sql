PRAGMA journal_mode = WAL;
PRAGMA foreign_keys = ON;

CREATE TABLE IF NOT EXISTS users (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    username      TEXT NOT NULL UNIQUE,
    password_hash TEXT NOT NULL,
    first_name    TEXT,
    last_name     TEXT,
    birthdate     TEXT,
    avatar_b64    TEXT
);

CREATE TABLE IF NOT EXISTS folders (
    id            INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id       INTEGER NOT NULL,
    parent_id     INTEGER,
    name          TEXT NOT NULL,
    next_uid      INTEGER NOT NULL DEFAULT 1,
    is_subscribed INTEGER NOT NULL DEFAULT 0,
    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
    FOREIGN KEY (parent_id) REFERENCES folders(id) ON DELETE CASCADE
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
CREATE INDEX IF NOT EXISTS idx_messages_user_id           ON messages(user_id);
CREATE INDEX IF NOT EXISTS idx_messages_msg_id_header     ON messages(message_id_header);
CREATE INDEX IF NOT EXISTS idx_folders_user_id            ON folders(user_id);
CREATE INDEX IF NOT EXISTS idx_folders_parent_id ON folders(parent_id);

CREATE TABLE IF NOT EXISTS recipients (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    address    TEXT NOT NULL,
    type       TEXT NOT NULL CHECK(type IN ('to', 'cc', 'bcc', 'reply-to')),
    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_recipients_message ON recipients(message_id);