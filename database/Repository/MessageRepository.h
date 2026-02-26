#pragma once

#include <vector>
#include <optional>
#include <string>
#include <cstdint>

#include "../Entity/Message.h"
#include "../Entity/Folder.h"
#include "../Entity/Attachment.h"
#include "../Entity/Recipient.h"

#include "../DAL/MessageDAL.h"
#include "../DAL/FolderDAL.h"
#include "../DAL/AttachmentDAL.h"
#include "../DAL/RecipientDAL.h"

#include "../DataBaseManager.h"

/*Error handling — always check the bool return and call getLastError() on failure
id is nullopt until persisted — don't use msg.id.value() before a successful insert
Trash before purge — hardDelete only works if the message is already Deleted status*/

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

    std::optional<Folder> findFolderByID(int64_t id) const;
    std::vector<Folder>   findFoldersByUser(int64_t user_id) const;
    std::optional<Folder> findFolderByName(int64_t user_id, const std::string& name) const;
    bool createFolder(Folder& folder);
    bool renameFolder(int64_t id, const std::string& new_name);
    bool deleteFolder(int64_t id);

    std::optional<Attachment> findAttachmentByID(int64_t id) const;
    std::vector<Attachment>   findAttachmentsByMessage(int64_t message_id) const;
    bool addAttachment(Attachment& attachment);
    bool updateAttachment(const Attachment& attachment);
    bool removeAttachment(int64_t id);

    std::optional<Recipient> findRecipientByID(int64_t id) const;
    std::vector<Recipient>   findRecipientsByMessage(int64_t message_id) const;
    bool addRecipient(Recipient& recipient);
    bool addRecipients(std::vector<Recipient>& recipients);
    bool replaceRecipients(int64_t message_id, std::vector<Recipient>& recipients);
    bool removeRecipient(int64_t id);
    bool removeAllRecipients(int64_t message_id);

    const std::string& getLastError() const;

private:
    MessageDAL m_message_dal;
    FolderDAL m_folder_dal;
    AttachmentDAL m_attachment_dal;
    RecipientDAL m_recipient_dal;
    mutable std::string m_last_error;

    bool setError(const std::string& msg) const;
};