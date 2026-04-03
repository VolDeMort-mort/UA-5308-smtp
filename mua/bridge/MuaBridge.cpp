#include "MuaBridge.hpp"

#include <QMetaObject>

namespace
{
QVariantList ToVariantList(const std::vector<std::string>& values)
{
	QVariantList out;
	out.reserve(static_cast<int>(values.size()));
	for (const auto& value : values)
	{
		out.push_back(QString::fromStdString(value));
	}

	return out;
}
} // namespace

MuaBridge::MuaBridge(QObject* parent)
    : QObject(parent)
    , m_worker(m_queue, [this](MailResult result) {
          QMetaObject::invokeMethod(this, [this, result = std::move(result)]() mutable {
              onResult(std::move(result));
          }, Qt::QueuedConnection);
      })
{
	m_worker.start();
}

MuaBridge::~MuaBridge()
{
	m_worker.stop();
}

bool MuaBridge::isConnected() const
{
	return m_isConnected;
}

bool MuaBridge::isBusy() const
{
	return m_isBusy;
}

bool MuaBridge::isReconnecting() const
{
	return m_isReconnecting;
}

int MuaBridge::reconnectAttempt() const
{
	return m_reconnectAttempt;
}

QString MuaBridge::reconnectMessage() const
{
	return m_reconnectMessage;
}

QString MuaBridge::lastError() const
{
	return m_lastError;
}

QVariantList MuaBridge::folders() const
{
	return m_folders;
}

QVariantList MuaBridge::mails() const
{
	return m_mails;
}

QVariantMap MuaBridge::currentMail() const
{
	return m_currentMail;
}

QString MuaBridge::lastDownloadedPath() const
{
	return m_lastDownloadedPath;
}

void MuaBridge::connectToServer(const QString& smtpHost, int smtpPort,
                                const QString& imapHost, int imapPort,
                                const QString& login, const QString& password)
{
	setBusy(true);
	setLastError(QString());

	ConnectCommand cmd;
	cmd.SMTPhost  = smtpHost.toStdString();
	cmd.SMTPport  = static_cast<uint16_t>(smtpPort);
	cmd.IMAPhost  = imapHost.toStdString();
	cmd.IMAPport  = static_cast<uint16_t>(imapPort);
	cmd.username  = login.toStdString();
	cmd.password  = password.toStdString();
	m_queue.push(cmd);
}

void MuaBridge::disconnectFromServer()
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(DisconnectCommand{});
}

void MuaBridge::sendMail(const QString& from, const QStringList& to,
                         const QString& subject, const QString& plainBody,
                         const QStringList& cc,
                         const QStringList& bcc,
                         const QString& htmlBody,
                         const QStringList& attachmentPaths)
{
	setBusy(true);
	setLastError(QString());

	SendMailCommand cmd;
	cmd.sender  = from.toStdString();
	cmd.subject = subject.toStdString();
	cmd.body    = plainBody.toStdString();
	cmd.htmlBody = htmlBody.toStdString();

	for (const auto& r : to)
	{
		const std::string addr = r.toStdString();
		cmd.to.push_back(addr);
		cmd.recipients.push_back(addr);
	}

	for (const auto& r : cc)
	{
		cmd.cc.push_back(r.toStdString());
	}

	for (const auto& r : bcc)
	{
		cmd.bcc.push_back(r.toStdString());
	}

	for (const auto& p : attachmentPaths)
	{
		cmd.attachments.push_back(p.toStdString());
	}

	m_queue.push(cmd);
}

void MuaBridge::fetchMails(const QString& folderName, int offset, int limit)
{
	setBusy(true);
	setLastError(QString());

	m_queue.push(FetchMailsCommand{
	    folderName.toStdString(),
	    static_cast<unsigned int>(offset),
	    static_cast<unsigned int>(limit)});
}

void MuaBridge::fetchMail(int mailId)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(FetchMailCommand{static_cast<int64_t>(mailId)});
}

void MuaBridge::deleteMail(int mailId)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(DeleteMailCommand{static_cast<int64_t>(mailId)});
}

void MuaBridge::markMailRead(int mailId)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(MarkMailReadCommand{static_cast<int64_t>(mailId)});
}

void MuaBridge::moveMail(int mailId, const QString& targetFolder)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(MoveMailCommand{static_cast<int64_t>(mailId), targetFolder.toStdString()});
}

void MuaBridge::downloadAttachment(int mailId, int attachmentIndex, const QString& outputPath)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(DownloadAttachmentCommand{
	    static_cast<int64_t>(mailId),
	    static_cast<std::size_t>(attachmentIndex),
	    outputPath.toStdString()});
}

void MuaBridge::fetchFolders()
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(FetchFoldersCommand{});
}

void MuaBridge::createFolder(const QString& name)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(CreateFolderCommand{name.toStdString()});
}

void MuaBridge::deleteFolder(const QString& folderName)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(DeleteFolderCommand{folderName.toStdString()});
}

void MuaBridge::addLabel(int mailId, const QString& label)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(AddLabelCommand{static_cast<int64_t>(mailId), label.toStdString()});
}

void MuaBridge::removeLabel(int mailId, const QString& label)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(RemoveLabelCommand{static_cast<int64_t>(mailId), label.toStdString()});
}

void MuaBridge::markMailStarred(int mailId, bool starred)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(MarkMailStarredCommand{static_cast<int64_t>(mailId), starred});
}

void MuaBridge::markMailImportant(int mailId, bool important)
{
	setBusy(true);
	setLastError(QString());
	m_queue.push(MarkMailImportantCommand{static_cast<int64_t>(mailId), important});
}

void MuaBridge::onResult(MailResult result)
{
	if (!std::holds_alternative<ReconnectStateResult>(result))
	{
		setBusy(false);
	}

	std::visit([this](const auto& r) {
		using T = std::decay_t<decltype(r)>;

		if constexpr (std::is_same_v<T, ConnectResult>)
		{
			if (r.success)
			{
				m_isReconnecting = false;
				m_reconnectAttempt = 0;
				m_reconnectMessage.clear();
				emit reconnectingChanged();
				emit reconnectAttemptChanged();
				emit reconnectMessageChanged();
				m_isConnected = true;
				emit connectionStateChanged();
				setLastError(QString());
				emit connected();
			}
			else
			{
				m_isConnected = false;
				emit connectionStateChanged();
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
		else if constexpr (std::is_same_v<T, DisconnectResult>)
		{
			if (r.success)
			{
				m_isConnected = false;
				emit connectionStateChanged();
				emit disconnected();
			}
			else
			{
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
		else if constexpr (std::is_same_v<T, SendMailResult>)
		{
			if (r.success)
			{
				setLastError(QString());
				emit mailSent();
			}
			else
			{
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
		else if constexpr (std::is_same_v<T, FetchMailsResult>)
		{
			if (r.success)
			{
				QVariantList items;
				items.reserve(static_cast<int>(r.mails.size()));
				for (const auto& mail : r.mails)
				{
					QVariantMap item;
					item.insert("mailId", static_cast<qlonglong>(mail.mailId));
					item.insert("isSeen", mail.isSeen);
					item.insert("isFlagged", mail.isFlagged);
					item.insert("subject", QString::fromStdString(mail.subject));
					item.insert("preview", QString::fromStdString(mail.preview));
					item.insert("date", QString::fromStdString(mail.date));
					items.push_back(item);
				}

				m_mails = std::move(items);
				emit mailsChanged();
				emit mailsFetched();
			}
			else
			{
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
		else if constexpr (std::is_same_v<T, FetchMailResult>)
		{
			if (r.success)
			{
				QVariantMap mail;
				mail.insert("mailId", static_cast<qlonglong>(r.mail.mailId));
				mail.insert("isSeen", r.mail.isSeen);
				mail.insert("isFlagged", r.mail.isFlagged);
				mail.insert("sender", QString::fromStdString(r.mail.sender));
				mail.insert("to", ToVariantList(r.mail.to));
				mail.insert("cc", ToVariantList(r.mail.cc));
				mail.insert("bcc", ToVariantList(r.mail.bcc));
				mail.insert("replyTo", ToVariantList(r.mail.replyTo));
				mail.insert("subject", QString::fromStdString(r.mail.subject));
				mail.insert("date", QString::fromStdString(r.mail.date));
				mail.insert("messageId", QString::fromStdString(r.mail.messageId));
				mail.insert("inReplyTo", QString::fromStdString(r.mail.inReplyTo));
				mail.insert("references", QString::fromStdString(r.mail.references));
				mail.insert("plainBody", QString::fromStdString(r.mail.plainBody));
				mail.insert("htmlBody", QString::fromStdString(r.mail.htmlBody));
				mail.insert("body", QString::fromStdString(r.mail.body));

				QVariantList attachments;
				attachments.reserve(static_cast<int>(r.mail.attachments.size()));
				for (const auto& attachment : r.mail.attachments)
				{
					QVariantMap item;
					item.insert("fileName", QString::fromStdString(attachment.fileName));
					item.insert("mimeType", QString::fromStdString(attachment.mimeType));
					item.insert("sizeBytes", static_cast<qlonglong>(attachment.sizeBytes));
					attachments.push_back(item);
				}
				mail.insert("attachments", attachments);

				m_currentMail = std::move(mail);
				emit currentMailChanged();
				emit mailFetched();
			}
			else
			{
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
		else if constexpr (std::is_same_v<T, FetchFoldersResult>)
		{
			if (r.success)
			{
				QVariantList names;
				names.reserve(static_cast<int>(r.folders.size()));
				for (const auto& name : r.folders)
				{
					names.push_back(QString::fromStdString(name));
				}

				m_folders = std::move(names);
				emit foldersChanged();
				emit foldersFetched();
			}
			else
			{
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
		else if constexpr (std::is_same_v<T, DownloadAttachmentResult>)
		{
			if (r.success)
			{
				m_lastDownloadedPath = QString::fromStdString(r.outputPath);
				emit attachmentDownloaded(m_lastDownloadedPath);
			}
			else
			{
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
		else if constexpr (std::is_same_v<T, ReconnectStateResult>)
		{
			setBusy(r.reconnecting);
			const bool hasreconnectingChanged = (m_isReconnecting != r.reconnecting);
			const int newAttempt = static_cast<int>(r.attempt);
			const bool attemptChanged = (m_reconnectAttempt != newAttempt);
			const QString newMessage = QString::fromStdString(r.message);
			const bool messageChanged = (m_reconnectMessage != newMessage);

			m_isReconnecting = r.reconnecting;
			m_reconnectAttempt = newAttempt;
			m_reconnectMessage = newMessage;

			if (r.reconnecting && m_isConnected)
			{
				m_isConnected = false;
				emit connectionStateChanged();
			}
			if (!r.reconnecting && !m_isConnected && !newMessage.isEmpty() && newMessage != "Reconnect canceled")
			{
				m_isConnected = true;
				emit connectionStateChanged();
			}

			if (hasreconnectingChanged)
			{
				emit reconnectingChanged();
			}
			if (attemptChanged)
			{
				emit reconnectAttemptChanged();
			}
			if (messageChanged)
			{
				emit reconnectMessageChanged();
			}
		}
		else if constexpr (std::is_same_v<T, MailActionResult>)
		{
			if (r.success)
			{
				emit actionDone();
			}
			else
			{
				setLastError(QString::fromStdString(r.error));
				emit errorOccurred(QString::fromStdString(r.error));
			}
		}
	}, result);
}

void MuaBridge::setBusy(bool value)
{
	if (m_isBusy == value)
	{
		return;
	}

	m_isBusy = value;
	emit busyChanged();
}

void MuaBridge::setLastError(const QString& value)
{
	if (m_lastError == value)
	{
		return;
	}

	m_lastError = value;
	emit lastErrorChanged();
}
