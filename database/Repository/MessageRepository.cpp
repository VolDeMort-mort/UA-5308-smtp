#include "MessageRepository.h"

MessageRepository::MessageRepository(DataBaseManager& db) : m_message_dal(db.getDB()) {};

bool MessageRepository::setError(const std::string& msg) const
{
    m_last_error = msg;
    return false;
}

const std::string& MessageRepository::getLastError() const
{
    return m_last_error;
}

std::optional<Message> MessageRepository::findByID(int64_t id) const
{
    return m_message_dal.findByID(id);
}

std::vector<Message> MessageRepository::findByUser(int64_t user_id) const
{
    return m_message_dal.findByUser(user_id);
}

std::vector<Message> MessageRepository::findByFolder(int64_t folder_id) const
{
    return m_message_dal.findByFolder(folder_id);
}

std::vector<Message> MessageRepository::findByStatus(int64_t user_id,
                                                      MessageStatus status) const
{
    return m_message_dal.findByStatus(user_id, status);
}

std::vector<Message> MessageRepository::findUnseen(int64_t user_id) const
{
    return m_message_dal.findUnseen(user_id);
}

std::vector<Message> MessageRepository::findStarred(int64_t user_id) const
{
    return m_message_dal.findStarred(user_id);
}

std::vector<Message> MessageRepository::findImportant(int64_t user_id) const
{
    return m_message_dal.findImportant(user_id);
}

std::vector<Message> MessageRepository::search(int64_t user_id,
                                                const std::string& query) const
{
    return m_message_dal.search(user_id, query);
}

bool MessageRepository::saveDraft(Message& msg)
{
    msg.status  = MessageStatus::Draft;
    msg.is_seen = true;

    return m_message_dal.insert(msg);
}

bool MessageRepository::editDraft(const Message& msg)
{
    if (!msg.id.has_value())
        return setError("editDraft: message has no id");

    auto existing = m_message_dal.findByID(msg.id.value());
    if (!existing.has_value())
        return setError("editDraft: message not found");

    if (existing->status != MessageStatus::Draft)
        return setError("editDraft: only Draft messages can be edited");

    return m_message_dal.update(msg);
}

bool MessageRepository::send(Message& msg)
{
    if (msg.id.has_value())
    {
        auto existing = m_message_dal.findByID(msg.id.value());
        if (!existing.has_value())
            return setError("send: message not found");

        if (existing->status != MessageStatus::Draft)
            return setError("send: only Draft messages can be sent");

        msg.status  = MessageStatus::Sent;
        msg.is_seen = true;
        return m_message_dal.update(msg);
    }

    // No id — insert directly as Sent
    msg.status  = MessageStatus::Sent;
    msg.is_seen = true;
    return m_message_dal.insert(msg);
}

bool MessageRepository::deliver(Message& msg)
{
    msg.status  = MessageStatus::Received;
    msg.is_seen = false;

    return m_message_dal.insert(msg);
}

bool MessageRepository::markSeen(int64_t id, bool seen)
{
    return m_message_dal.updateSeen(id, seen);
}

bool MessageRepository::markStarred(int64_t id, bool starred)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("markStarred: message not found");

    return m_message_dal.updateFlags(id, msg->is_seen, starred, msg->is_important);
}

bool MessageRepository::markImportant(int64_t id, bool important)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("markImportant: message not found");

    return m_message_dal.updateFlags(id, msg->is_seen, msg->is_starred, important);
}

bool MessageRepository::moveToFolder(int64_t id, std::optional<int64_t> folder_id)
{
    return m_message_dal.moveToFolder(id, folder_id);
}

bool MessageRepository::moveToTrash(int64_t id)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("moveToTrash: message not found");

    if (msg->status == MessageStatus::Deleted)
        return setError("moveToTrash: message is already deleted");

    return m_message_dal.softDelete(id);
}

bool MessageRepository::restore(int64_t id)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("restore: message not found");

    if (msg->status != MessageStatus::Deleted)
        return setError("restore: only Deleted messages can be restored");

    MessageStatus restored = msg->receiver.empty()
                             ? MessageStatus::Sent
                             : MessageStatus::Received;

    return m_message_dal.updateStatus(id, restored);
}

bool MessageRepository::hardDelete(int64_t id)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("HardDelete: message not found");

    if (msg->status != MessageStatus::Deleted)
        return setError("HardDelete: message must be Deleted before it can be HardDeleted");

    return m_message_dal.hardDelete(id);
}