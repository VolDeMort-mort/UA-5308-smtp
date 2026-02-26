#include "MessageRepository.h"

MessageRepository::MessageRepository(DataBaseManager& db)
    : m_message_dal(db.getDB())
    , m_folder_dal(db.getDB())
    , m_attachment_dal(db.getDB())
    , m_recipient_dal(db.getDB()) 
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

std::optional<Folder> MessageRepository::findFolderByID(int64_t id) const
{
    return m_folder_dal.findByID(id);
}

std::vector<Folder> MessageRepository::findFoldersByUser(int64_t user_id) const
{
    return m_folder_dal.findByUser(user_id);
}

std::optional<Folder> MessageRepository::findFolderByName(int64_t user_id,
                                                           const std::string& name) const
{
    return m_folder_dal.findByName(user_id, name);
}

bool MessageRepository::createFolder(Folder& folder)
{
    if (folder.name.empty())
        return setError("createFolder: folder name cannot be empty");

    auto existing = m_folder_dal.findByName(folder.user_id, folder.name);
    if (existing.has_value())
        return setError("createFolder: folder '" + folder.name + "' already exists");

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
    return m_folder_dal.update(folder.value());
}

bool MessageRepository::deleteFolder(int64_t id)
{
    auto folder = m_folder_dal.findByID(id);
    if (!folder.has_value())
        return setError("deleteFolder: folder not found");

    return m_folder_dal.hardDelete(id);
}

std::optional<Attachment> MessageRepository::findAttachmentByID(int64_t id) const
{
    return m_attachment_dal.findByID(id);
}

std::vector<Attachment> MessageRepository::findAttachmentsByMessage(int64_t message_id) const
{
    return m_attachment_dal.findByMessage(message_id);
}

bool MessageRepository::addAttachment(Attachment& attachment)
{
    auto msg = m_message_dal.findByID(attachment.message_id);
    if (!msg.has_value())
        return setError("addAttachment: message not found");

    if (attachment.file_name.empty())
        return setError("addAttachment: file_name cannot be empty");

    if (attachment.storage_path.empty())
        return setError("addAttachment: storage_path cannot be empty");

    return m_attachment_dal.insert(attachment);
}

bool MessageRepository::updateAttachment(const Attachment& attachment)
{
    if (!attachment.id.has_value())
        return setError("updateAttachment: attachment has no id");

    auto existing = m_attachment_dal.findByID(attachment.id.value());
    if (!existing.has_value())
        return setError("updateAttachment: attachment not found");

    return m_attachment_dal.update(attachment);
}

bool MessageRepository::removeAttachment(int64_t id)
{
    auto existing = m_attachment_dal.findByID(id);
    if (!existing.has_value())
        return setError("removeAttachment: attachment not found");

    return m_attachment_dal.hardDelete(id);
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
    auto msg = m_message_dal.findByID(recipient.message_id);
    if (!msg.has_value())
        return setError("addRecipient: message not found");

    if (recipient.address.empty())
        return setError("addRecipient: address cannot be empty");

    return m_recipient_dal.insert(recipient);
}

bool MessageRepository::removeRecipient(int64_t id)
{
    auto existing = m_recipient_dal.findByID(id);
    if (!existing.has_value())
        return setError("removeRecipient: recipient not found");

    return m_recipient_dal.hardDelete(id);
}