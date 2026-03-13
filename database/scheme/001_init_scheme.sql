-- ============================================================
-- 002_imap_optimized_scheme.sql
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
-- id (глобальний) ідеально підходить як UIDVALIDITY для IMAP
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS folders (
    id       INTEGER PRIMARY KEY AUTOINCREMENT, 
    user_id  INTEGER NOT NULL,
    name     TEXT NOT NULL,
    next_uid INTEGER NOT NULL DEFAULT 1, -- Локальний лічильник UID для цієї папки
    is_subscribed BOOL NOT NULL DEFAULT FALSE

    FOREIGN KEY (user_id) REFERENCES users(id) ON DELETE CASCADE
);

-- ------------------------------------------------------------
-- Messages (Тільки метадані, сам текст і вкладення у файлі)
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS messages (
    id                INTEGER PRIMARY KEY AUTOINCREMENT, -- Для внутрішніх JOIN'ів
    user_id           INTEGER NOT NULL,                  -- Власник цієї копії листа
    folder_id         INTEGER NOT NULL,
    uid               INTEGER NOT NULL,                  -- Унікальний ID в межах папки (IMAP UID)

    -- Посилання на фізичний файл
    raw_file_path     TEXT NOT NULL,                     
    size_bytes        INTEGER NOT NULL,                  -- Загальний розмір файлу (для RFC822.SIZE)
    mime_structure    TEXT                               -- BODYSTRUCTURE

    -- Дані конверта (ENVELOPE)
    message_id_header TEXT,                              -- Глобальний ID (напр. <123@domain.com>)
    in_reply_to       TEXT,                              -- Для побудови дерева діалогів
    references_header TEXT,                              -- Весь ланцюжок діалогу (усі ID через пробіл)
    from_address      TEXT NOT NULL,                     -- Хто написав
    sender_address    TEXT,                              -- Хто відправив фізично (якщо відрізняється)
    subject           TEXT,                              -- Тема листа
    
    -- IMAP Прапорці (Flags)
    is_seen           INTEGER NOT NULL DEFAULT 0,        -- \Seen
    is_deleted        INTEGER NOT NULL DEFAULT 0,        -- \Deleted
    is_draft          INTEGER NOT NULL DEFAULT 0,        -- \Draft
    is_answered       INTEGER NOT NULL DEFAULT 0,        -- \Answered
    is_flagged        INTEGER NOT NULL DEFAULT 0,        -- \Flagged (було Starred)
    is_recent         INTEGER NOT NULL DEFAULT 1,        -- \Recent (Щойно доставлено)

    -- Дати
    internal_date     TEXT DEFAULT (datetime('now')),    -- Дата отримання сервером (не змінюється при COPY/MOVE)
    date_header       TEXT,                              -- Дата, яку вказав відправник у самому листі

    FOREIGN KEY (user_id)   REFERENCES users(id)   ON DELETE CASCADE,
    FOREIGN KEY (folder_id) REFERENCES folders(id) ON DELETE CASCADE
);

-- ------------------------------------------------------------
-- Recipients (To, Cc, Bcc, Reply-To)
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS recipients (
    id         INTEGER PRIMARY KEY AUTOINCREMENT,
    message_id INTEGER NOT NULL,
    address    TEXT NOT NULL,
    type       TEXT NOT NULL CHECK(type IN ('to', 'cc', 'bcc', 'reply-to')),

    FOREIGN KEY (message_id) REFERENCES messages(id) ON DELETE CASCADE
);

-- ------------------------------------------------------------
-- Indexes (Критично важливо для швидкості IMAP)
-- ------------------------------------------------------------
-- Гарантуємо, що в одній папці не може бути двох однакових UID
CREATE UNIQUE INDEX IF NOT EXISTS idx_messages_folder_uid ON messages(folder_id, uid);

CREATE INDEX IF NOT EXISTS idx_messages_user_id           ON messages(user_id);
CREATE INDEX IF NOT EXISTS idx_messages_msg_id_header     ON messages(message_id_header);
CREATE INDEX IF NOT EXISTS idx_recipients_message         ON recipients(message_id);
CREATE INDEX IF NOT EXISTS idx_folders_user_id            ON folders(user_id);