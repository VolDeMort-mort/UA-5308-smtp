#pragma once

#include <QObject>
#include <QString>
#include <QStringList>

#include "../core/CommandQueue.hpp"
#include "../core/MailResult.hpp"
#include "../core/MailWorker.hpp"

class MuaBridge : public QObject
{
	Q_OBJECT

public:
	explicit MuaBridge(QObject* parent = nullptr);
	~MuaBridge();

	// called from QML via Q_INVOKABLE
	Q_INVOKABLE void connectToServer(const QString& smtpHost, int smtpPort,
	                                 const QString& imapHost, int imapPort,
	                                 const QString& login, const QString& password);

	Q_INVOKABLE void disconnectFromServer();

	Q_INVOKABLE void sendMail(const QString& from,
	                          const QStringList& to,
	                          const QString& subject,
	                          const QString& body);

	Q_INVOKABLE void fetchMails(const QString& folderName, int offset, int limit);
	Q_INVOKABLE void fetchMail(int mailId);
	Q_INVOKABLE void deleteMail(int mailId);
	Q_INVOKABLE void markMailRead(int mailId);

	Q_INVOKABLE void fetchFolders();
	Q_INVOKABLE void createFolder(const QString& name);
	Q_INVOKABLE void deleteFolder(const QString& folderName);

	Q_INVOKABLE void addLabel(int mailId, const QString& label);
	Q_INVOKABLE void removeLabel(int mailId, const QString& label);
	Q_INVOKABLE void markMailStarred(int mailId, bool starred);
	Q_INVOKABLE void markMailImportant(int mailId, bool important);

signals:
	// result signals — QML connects to these
	void connected();
	void disconnected();
	void mailSent();
	void errorOccurred(const QString& message);

	// data signals — emitted when fetch completes
	// TODO: replace with proper Qt models when ready
	void mailsFetched();
	void mailFetched();
	void foldersFetched();

	void actionDone(); // generic signal for simple actions

private:
	// called from the worker thread; result is dispatched to UI thread via Qt
	void onResult(MailResult result);

	CommandQueue m_queue;
	MailWorker m_worker;
};
