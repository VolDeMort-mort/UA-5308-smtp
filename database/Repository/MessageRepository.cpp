#include "MessageRepository.h"
#include <climits>

MessageRepository::MessageRepository(DataBaseManager& db)
    : m_db(db)
    , m_message_dal(db.getDB(), db.pool())
    , m_folder_dal(db.getDB(), db.pool())
    , m_recipient_dal(db.getDB(), db.pool())
{}

bool MessageRepository::setError(const std::string& msg) const
{
    m_last_error = msg;
    return false;
}

const std::string& MessageRepository::getLastError() const
{
    return m_last_error;
}

bool MessageRepository::assignUID(Message& msg, int64_t folder_id)
{
    auto lock = m_db.writeLock();
    Transaction tx(m_db.getDB());

    if (!tx.valid()) return setError("assignUID: failed to begin transaction");

    // Fetch folder INSIDE transaction to prevent race condition
    auto folder = m_folder_dal.findByID(folder_id);
    if (!folder.has_value())
        return setError("assignUID: folder not found");

    msg.folder_id = folder_id;
    msg.uid       = folder->next_uid;

    if (!m_message_dal.insert(msg))
        return setError(m_message_dal.getLastError());

    if (!m_folder_dal.incrementNextUID(folder_id))
        return setError(m_folder_dal.getLastError());

    if (!tx.commit())
        return setError("assignUID: commit failed");

    return true;
}

std::optional<Message> MessageRepository::findByID(int64_t id) const
{
    return m_message_dal.findByID(id);
}

std::optional<Message> MessageRepository::findByUID(int64_t folder_id, int64_t uid) const
{
    return m_message_dal.findByUID(folder_id, uid);
}

std::vector<Message> MessageRepository::findByUser(int64_t user_id, int limit, int offset) const
{
    return m_message_dal.findByUser(user_id, limit, offset);
}

std::vector<Message> MessageRepository::findByFolder(int64_t folder_id, int limit, int offset) const
{
    return m_message_dal.findByFolder(folder_id, limit, offset);
}

std::vector<Message> MessageRepository::findUnseen(int64_t folder_id, int limit, int offset) const
{
    return m_message_dal.findUnseen(folder_id, limit, offset);
}

std::vector<Message> MessageRepository::findDeleted(int64_t folder_id, int limit, int offset) const
{
    return m_message_dal.findDeleted(folder_id, limit, offset);
}

std::vector<Message> MessageRepository::findFlagged(int64_t folder_id, int limit, int offset) const
{
    return m_message_dal.findFlagged(folder_id, limit, offset);
}

std::vector<Message> MessageRepository::search(int64_t user_id, const std::string& query, int limit, int offset) const
{
    return m_message_dal.search(user_id, query, limit, offset);
}

bool MessageRepository::deliver(Message& msg, int64_t folder_id)
{
    if (folder_id <= 0)
    {
        auto inbox = m_folder_dal.findByName(msg.user_id, "INBOX");
        if (!inbox.has_value())
            return setError("deliver: INBOX not found for user");
        folder_id = inbox->id.value();
    }

    msg.is_seen   = false;
    msg.is_recent = true;
    msg.is_draft  = false;

    return assignUID(msg, folder_id);
}

bool MessageRepository::saveToFolder(Message& msg, int64_t folder_id)
{
    msg.is_seen   = true;
    msg.is_recent = false;
    return assignUID(msg, folder_id);
}

bool MessageRepository::markSeen(int64_t id, bool seen)
{
    auto lock = m_db.writeLock();
    return m_message_dal.updateSeen(id, seen);
}

bool MessageRepository::markDeleted(int64_t id, bool deleted)
{
    auto lock = m_db.writeLock();
    return m_message_dal.updateDeleted(id, deleted);
}

bool MessageRepository::markFlagged(int64_t id, bool flagged)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("markFlagged: message not found");

    auto lock = m_db.writeLock();
    return m_message_dal.updateFlags(id, msg->is_seen, msg->is_deleted, msg->is_draft,
                                     msg->is_answered, flagged, msg->is_recent);
}

bool MessageRepository::markAnswered(int64_t id, bool answered)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("markAnswered: message not found");

    auto lock = m_db.writeLock();
    return m_message_dal.updateFlags(id, msg->is_seen, msg->is_deleted, msg->is_draft,
                                     answered, msg->is_flagged, msg->is_recent);
}

bool MessageRepository::markDraft(int64_t id, bool draft)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("markDraft: message not found");

    auto lock = m_db.writeLock();
    return m_message_dal.updateFlags(id, msg->is_seen, msg->is_deleted, draft,
                                     msg->is_answered, msg->is_flagged, msg->is_recent);
}

bool MessageRepository::updateFlags(int64_t id, bool is_seen, bool is_deleted, bool is_draft,
                                    bool is_answered, bool is_flagged, bool is_recent)
{
    auto lock = m_db.writeLock();
    return m_message_dal.updateFlags(id, is_seen, is_deleted, is_draft, is_answered, is_flagged, is_recent);
}

bool MessageRepository::moveToFolder(int64_t id, int64_t folder_id)
{
    auto folder = m_folder_dal.findByID(folder_id);
    if (!folder.has_value())
        return setError("moveToFolder: folder not found");

    auto lock = m_db.writeLock();
    Transaction tx(m_db.getDB());
    if (!tx.valid()) return setError("moveToFolder: failed to begin transaction");

    if (!m_message_dal.moveToFolder(id, folder_id, folder->next_uid))
        return setError(m_message_dal.getLastError());

    if (!m_folder_dal.incrementNextUID(folder_id))
        return setError(m_folder_dal.getLastError());

    if (!tx.commit())
        return setError("moveToFolder: commit failed");

    return true;
}

bool MessageRepository::expunge(int64_t folder_id)
{
    auto deleted = m_message_dal.findDeleted(folder_id, INT_MAX, 0);

    auto lock = m_db.writeLock();
    Transaction tx(m_db.getDB());
    if (!tx.valid()) return setError("expunge: failed to begin transaction");

    for (const auto& msg : deleted)
    {
        if (!msg.id.has_value()) continue;
        if (!m_message_dal.hardDelete(msg.id.value()))
            return setError(m_message_dal.getLastError());
    }

    if (!tx.commit())
        return setError("expunge: commit failed");

    return true;
}

bool MessageRepository::hardDelete(int64_t id)
{
    if (!m_message_dal.findByID(id).has_value())
        return setError("hardDelete: message not found");

    auto lock = m_db.writeLock();
    return m_message_dal.hardDelete(id);
}

std::optional<Message> MessageRepository::copy(int64_t id, int64_t target_folder_id)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
    {
        setError("copy: message not found");
        return std::nullopt;
    }

    auto folder = m_folder_dal.findByID(target_folder_id);
    if (!folder.has_value())
    {
        setError("copy: target folder not found");
        return std::nullopt;
    }

    auto recipients = m_recipient_dal.findByMessage(id);

    Message copy = msg.value();
    copy.id = std::nullopt;
    copy.uid  = 0;
    copy.folder_id = target_folder_id;
    copy.uid = folder->next_uid;

    auto lock = m_db.writeLock();
    Transaction tx(m_db.getDB());
    if (!tx.valid())
    {
        setError("copy: failed to begin transaction");
        return std::nullopt;
    }

    if (!m_message_dal.insert(copy))
    {
        setError(m_message_dal.getLastError());
        return std::nullopt;
    }

    for (auto& r : recipients)
    {
        r.id = std::nullopt;
        r.message_id = copy.id.value();

        if (!m_recipient_dal.insert(r))
        {
            setError("copy: failed to copy recipient — " + m_recipient_dal.getLastError());
            return std::nullopt;
        }
    }

    if (!m_folder_dal.incrementNextUID(target_folder_id))
    {
        setError(m_folder_dal.getLastError());
        return std::nullopt;
    }

    if (!tx.commit())
    {
        setError("copy: commit failed");
        return std::nullopt;
    }

    return copy;
}

std::optional<Folder> MessageRepository::findFolderByID(int64_t id) const
{
    return m_folder_dal.findByID(id);
}

std::vector<Folder> MessageRepository::findFoldersByUser(int64_t user_id, int limit, int offset) const
{
    return m_folder_dal.findByUser(user_id, limit, offset);
}

std::optional<Folder> MessageRepository::findFolderByName(int64_t user_id, const std::string& name) const
{
    return m_folder_dal.findByName(user_id, name);
}

bool MessageRepository::createFolder(Folder& folder)
{
    if (folder.name.empty())
        return setError("createFolder: folder name cannot be empty");

    if (m_folder_dal.findByName(folder.user_id, folder.name).has_value())
        return setError("createFolder: folder '" + folder.name + "' already exists");

    auto lock = m_db.writeLock();
    return m_folder_dal.insert(folder);
}

bool MessageRepository::renameFolder(int64_t id, const std::string& new_name)
{
    if (new_name.empty())
        return setError("renameFolder: new name cannot be empty");

    auto folder = m_folder_dal.findByID(id);
    if (!folder.has_value())
        return setError("renameFolder: folder not found");

    auto existing = m_folder_dal.findByName(folder->user_id, new_name);
    if (existing.has_value() && existing->id != folder->id)
        return setError("renameFolder: folder '" + new_name + "' already exists");

    folder->name = new_name;

    auto lock = m_db.writeLock();
    return m_folder_dal.update(folder.value());
}

bool MessageRepository::deleteFolder(int64_t id)
{
    if (!m_folder_dal.findByID(id).has_value())
        return setError("deleteFolder: folder not found");

    auto lock = m_db.writeLock();
    return m_folder_dal.hardDelete(id);
}

std::optional<Recipient> MessageRepository::findRecipientByID(int64_t id) const
{
    return m_recipient_dal.findByID(id);
}

std::vector<Recipient> MessageRepository::findRecipientsByMessage(int64_t message_id) const
{
    return m_recipient_dal.findByMessage(message_id);
}

bool MessageRepository::addRecipient(Recipient& recipient)
{
    if (!m_message_dal.findByID(recipient.message_id).has_value())
        return setError("addRecipient: message not found");

    if (recipient.address.empty())
        return setError("addRecipient: address cannot be empty");

    auto lock = m_db.writeLock();
    return m_recipient_dal.insert(recipient);
}

bool MessageRepository::removeRecipient(int64_t id)
{
    if (!m_recipient_dal.findByID(id).has_value())
        return setError("removeRecipient: recipient not found");

    auto lock = m_db.writeLock();
    return m_recipient_dal.hardDelete(id);
}

bool MessageRepository::setFlags(int64_t id, const std::vector<std::string>& flags)
{
    auto msg = m_message_dal.findByID(id);
    if (!msg.has_value())
        return setError("setFlags: message not found");

    bool is_seen     = msg->is_seen;
    bool is_deleted  = msg->is_deleted;
    bool is_draft    = msg->is_draft;
    bool is_answered = msg->is_answered;
    bool is_flagged  = msg->is_flagged;
    bool is_recent   = msg->is_recent;

    // If no flags provided, clear all flags
    if (flags.empty())
    {
        is_seen = is_deleted = is_draft = is_answered = is_flagged = is_recent = false;
    }
    else
    {
        for (const auto& flag : flags)
        {
            bool value = true;
            std::string name = flag;

            if (!flag.empty() && flag[0] == '-')
            {
                value = false;
                name  = flag.substr(1);
            }

            if      (name == "\\Seen")     is_seen     = value;
            else if (name == "\\Deleted")  is_deleted  = value;
            else if (name == "\\Draft")    is_draft    = value;
            else if (name == "\\Answered") is_answered = value;
            else if (name == "\\Flagged")  is_flagged  = value;
            else if (name == "\\Recent")   is_recent   = value;
            else return setError("setFlags: unknown flag '" + name + "'");
        }
    }

    auto lock = m_db.writeLock();
    return m_message_dal.updateFlags(id, is_seen, is_deleted, is_draft, is_answered, is_flagged, is_recent);
}

bool MessageRepository::append(Message& msg, int64_t folder_id)
{
    return assignUID(msg, folder_id);
}

bool MessageRepository::incrementNextUID(int64_t folder_id)
{
    if (!m_folder_dal.findByID(folder_id).has_value())
        return setError("incrementNextUID: folder not found");

    auto lock = m_db.writeLock();
    if (!m_folder_dal.incrementNextUID(folder_id))
        return setError(m_folder_dal.getLastError());

    return true;
}

bool MessageRepository::clearRecentByFolder(int64_t folder_id)
{
    if (!m_folder_dal.findByID(folder_id).has_value())
        return setError("clearRecentByFolder: folder not found");

    auto lock = m_db.writeLock();
    if (!m_message_dal.clearRecentByFolder(folder_id))
        return setError(m_message_dal.getLastError());

    return true;
}

bool MessageRepository::closeFolder(int64_t folder_id)
{
    if (!expunge(folder_id))
        return false;

    auto lock = m_db.writeLock();
    if (!m_message_dal.clearRecentByFolder(folder_id))
        return setError(m_message_dal.getLastError());

    return true;
}

bool MessageRepository::setSubscribed(int64_t folder_id, bool subscribed)
{
    if (!m_folder_dal.findByID(folder_id).has_value())
        return setError("setSubscribed: folder not found");

    auto lock = m_db.writeLock();
    if (!m_folder_dal.setSubscribed(folder_id, subscribed))
        return setError(m_folder_dal.getLastError());

    return true;
}

std::vector<Folder> MessageRepository::findFoldersByParent(int64_t parent_id, int limit, int offset) const
{
    return m_folder_dal.findByParent(parent_id, limit, offset);
}