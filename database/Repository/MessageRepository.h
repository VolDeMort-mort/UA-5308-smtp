#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>

#include "../Entity/Message.h"
#include "../DAL/MessageDAL.h"
#include "../DataBaseManager.h"

class MessageRepository
{
public:
    explicit MessageRepository(DataBaseManager& db);

    std::optional<Message> findByID(int64_t id) const;
    std::vector<Message>   findByUser(int64_t user_id) const;
    std::vector<Message>   findByFolder(int64_t folder_id) const;
    std::vector<Message>   findByStatus(int64_t user_id, MessageStatus status) const;
    std::vector<Message>   findUnseen(int64_t user_id) const;
    std::vector<Message>   findStarred(int64_t user_id) const;
    std::vector<Message>   findImportant(int64_t user_id) const;
    std::vector<Message>   search(int64_t user_id, const std::string& query) const;

    bool saveDraft(Message& msg);
    bool editDraft(const Message& msg);
    bool send(Message& msg);
    bool deliver(Message& msg);

    bool markSeen(int64_t id, bool seen);
    bool markStarred(int64_t id, bool starred);
    bool markImportant(int64_t id, bool important);

    bool moveToFolder(int64_t id, std::optional<int64_t> folder_id);
    bool moveToTrash(int64_t id);
    bool restore(int64_t id);
    bool hardDelete(int64_t id);

    const std::string& getLastError() const;

private:
    MessageDAL m_message_dal;
    mutable std::string m_last_error;

    bool setError(const std::string& msg) const;
};