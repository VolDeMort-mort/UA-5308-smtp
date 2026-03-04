#include "MuaBridge.hpp"

#include <QMetaObject>

MuaBridge::MuaBridge(QObject* parent)
    : QObject(parent)
    , m_worker(m_queue, [this](MailResult result) {
          // Callback is invoked from the worker thread;
          // QMetaObject::invokeMethod safely re-dispatches to the UI thread.
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

// -------------------------------------------------------------------
// Q_INVOKABLE methods called by QML
// Convert Qt types to C++ and push the command into the queue
// -------------------------------------------------------------------

void MuaBridge::connectToServer(const QString& smtpHost, int smtpPort,
                                const QString& imapHost, int imapPort,
                                const QString& login, const QString& password)
{
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
	m_queue.push(DisconnectCommand{});
}

void MuaBridge::sendMail(const QString& from, const QStringList& to,
                         const QString& subject, const QString& body)
{
	SendMailCommand cmd;
	cmd.sender  = from.toStdString();
	cmd.subject = subject.toStdString();
	cmd.body    = body.toStdString();
	for (const auto& r : to)
		cmd.recipients.push_back(r.toStdString());
	m_queue.push(cmd);
}

void MuaBridge::fetchMails(const QString& folderName, int offset, int limit)
{
	m_queue.push(FetchMailsCommand{
	    folderName.toStdString(),
	    static_cast<unsigned int>(offset),
	    static_cast<unsigned int>(limit)});
}

void MuaBridge::fetchMail(int mailId)
{
	m_queue.push(FetchMailCommand{static_cast<int64_t>(mailId)});
}

void MuaBridge::deleteMail(int mailId)
{
	m_queue.push(DeleteMailCommand{static_cast<int64_t>(mailId)});
}

void MuaBridge::markMailRead(int mailId)
{
	m_queue.push(MarkMailReadCommand{static_cast<int64_t>(mailId)});
}

void MuaBridge::fetchFolders()
{
	m_queue.push(FetchFoldersCommand{});
}

void MuaBridge::createFolder(const QString& name)
{
	m_queue.push(CreateFolderCommand{name.toStdString()});
}

void MuaBridge::deleteFolder(const QString& folderName)
{
	m_queue.push(DeleteFolderCommand{folderName.toStdString()});
}

void MuaBridge::addLabel(int mailId, const QString& label)
{
	m_queue.push(AddLabelCommand{static_cast<int64_t>(mailId), label.toStdString()});
}

void MuaBridge::removeLabel(int mailId, const QString& label)
{
	m_queue.push(RemoveLabelCommand{static_cast<int64_t>(mailId), label.toStdString()});
}

void MuaBridge::markMailStarred(int mailId, bool starred)
{
	m_queue.push(MarkMailStarredCommand{static_cast<int64_t>(mailId), starred});
}

void MuaBridge::markMailImportant(int mailId, bool important)
{
	m_queue.push(MarkMailImportantCommand{static_cast<int64_t>(mailId), important});
}

// -------------------------------------------------------------------
// Result handling — always called on the UI thread
// -------------------------------------------------------------------

void MuaBridge::onResult(MailResult result)
{
	std::visit([this](const auto& r) {
		using T = std::decay_t<decltype(r)>;

		if constexpr (std::is_same_v<T, ConnectResult>)
		{
			if (r.success) emit connected();
			else           emit errorOccurred(QString::fromStdString(r.error));
		}
		else if constexpr (std::is_same_v<T, DisconnectResult>)
		{
			if (r.success) emit disconnected();
			else           emit errorOccurred(QString::fromStdString(r.error));
		}
		else if constexpr (std::is_same_v<T, SendMailResult>)
		{
			if (r.success) emit mailSent();
			else           emit errorOccurred(QString::fromStdString(r.error));
		}
		else if constexpr (std::is_same_v<T, FetchMailsResult>)
		{
			if (r.success) emit mailsFetched();
			else           emit errorOccurred(QString::fromStdString(r.error));
		}
		else if constexpr (std::is_same_v<T, FetchMailResult>)
		{
			if (r.success) emit mailFetched();
			else           emit errorOccurred(QString::fromStdString(r.error));
		}
		else if constexpr (std::is_same_v<T, FetchFoldersResult>)
		{
			if (r.success) emit foldersFetched();
			else           emit errorOccurred(QString::fromStdString(r.error));
		}
		else if constexpr (std::is_same_v<T, MailActionResult>)
		{
			if (r.success) emit actionDone();
			else           emit errorOccurred(QString::fromStdString(r.error));
		}
	}, result);
}
