#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>

#include "../Entity/Message.h"
#include "../Entity/Folder.h"
#include "../Entity/Recipient.h"

#include "../DAL/MessageDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/RecipientDAL.h"

#include "../DataBaseManager.h"

class MessageRepository
{
public:
    explicit MessageRepository(DataBaseManager& db);
    explicit MessageRepository(sqlite3* db);

    std::optional<Message> findByID(int64_t id) const;
    std::optional<Message> findByUID(int64_t folder_id, int64_t uid) const;
    std::vector<Message> findByUser(int64_t user_id) const;
    std::vector<Message> findByFolder(int64_t folder_id) const;
    std::vector<Message> findUnseen(int64_t folder_id) const;
    std::vector<Message> findDeleted(int64_t folder_id) const;
    std::vector<Message> findFlagged(int64_t folder_id) const;
    std::vector<Message> search(int64_t user_id, const std::string& query) const;

    bool deliver(Message& msg, int64_t folder_id);
    bool saveToFolder(Message& msg, int64_t folder_id);

    bool markSeen(int64_t id, bool seen);
    bool markDeleted(int64_t id, bool deleted);
    bool markFlagged(int64_t id, bool flagged);
    bool markAnswered(int64_t id, bool answered);
    bool markDraft(int64_t id, bool draft);
    bool updateFlags(int64_t id, bool is_seen, bool is_deleted, bool is_draft,
                     bool is_answered, bool is_flagged, bool is_recent);

    bool moveToFolder(int64_t id, int64_t folder_id);
    bool expunge(int64_t folder_id);
    bool hardDelete(int64_t id);

    std::optional<Folder> findFolderByID(int64_t id) const;
    std::vector<Folder> findFoldersByUser(int64_t user_id) const;
    std::optional<Folder> findFolderByName(int64_t user_id, const std::string& name) const;
    bool createFolder(Folder& folder);
    bool renameFolder(int64_t id, const std::string& new_name);
    bool deleteFolder(int64_t id);

    std::optional<Recipient> findRecipientByID(int64_t id) const;
    std::vector<Recipient> findRecipientsByMessage(int64_t message_id) const;
    bool addRecipient(Recipient& recipient);
    bool removeRecipient(int64_t id);

    const std::string& getLastError() const;

    bool setFlags(int64_t id, const std::vector<std::string>& flags);
    bool append(Message& msg, int64_t folder_id);
    bool incrementNextUID(int64_t folder_id);
    std::optional<Message> copy(int64_t id, int64_t target_folder_id);

private:
    MessageDAL m_message_dal;
    FolderDAL m_folder_dal;
    RecipientDAL m_recipient_dal;
    mutable std::string m_last_error;

    bool assignUID(Message& msg, int64_t folder_id);
    bool setError(const std::string& msg) const;
};