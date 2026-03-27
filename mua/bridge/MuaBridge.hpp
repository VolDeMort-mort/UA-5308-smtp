#pragma once

#include <QObject>
#include <QVariantList>
#include <QVariantMap>
#include <QString>
#include <QStringList>

#include "../core/CommandQueue.hpp"
#include "../core/MailResult.hpp"
#include "../core/MailWorker.hpp"

class MuaBridge : public QObject
{
	Q_OBJECT
	Q_PROPERTY(bool isConnected READ isConnected NOTIFY connectionStateChanged)
	Q_PROPERTY(bool isBusy READ isBusy NOTIFY busyChanged)
	Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
	Q_PROPERTY(QVariantList folders READ folders NOTIFY foldersChanged)
	Q_PROPERTY(QVariantList mails READ mails NOTIFY mailsChanged)
	Q_PROPERTY(QVariantMap currentMail READ currentMail NOTIFY currentMailChanged)
	Q_PROPERTY(QString lastDownloadedPath READ lastDownloadedPath NOTIFY attachmentDownloaded)

public:
	explicit MuaBridge(QObject* parent = nullptr);
	~MuaBridge();

	bool isConnected() const;
	bool isBusy() const;
	QString lastError() const;
	QVariantList folders() const;
	QVariantList mails() const;
	QVariantMap currentMail() const;
	QString lastDownloadedPath() const;

	Q_INVOKABLE void connectToServer(const QString& smtpHost, int smtpPort,
	                                 const QString& imapHost, int imapPort,
	                                 const QString& login, const QString& password);

	Q_INVOKABLE void disconnectFromServer();

	Q_INVOKABLE void sendMail(const QString& from,
	                          const QStringList& to,
	                          const QString& subject,
	                          const QString& plainBody,
	                          const QStringList& cc = {},
	                          const QStringList& bcc = {},
	                          const QString& htmlBody = QString(),
	                          const QStringList& attachmentPaths = {});

	Q_INVOKABLE void fetchMails(const QString& folderName, int offset, int limit);
	Q_INVOKABLE void fetchMail(int mailId);
	Q_INVOKABLE void deleteMail(int mailId);
	Q_INVOKABLE void markMailRead(int mailId);
	Q_INVOKABLE void moveMail(int mailId, const QString& targetFolder);
	Q_INVOKABLE void downloadAttachment(int mailId, int attachmentIndex, const QString& outputPath = QString());

	Q_INVOKABLE void fetchFolders();
	Q_INVOKABLE void createFolder(const QString& name);
	Q_INVOKABLE void deleteFolder(const QString& folderName);

	Q_INVOKABLE void addLabel(int mailId, const QString& label);
	Q_INVOKABLE void removeLabel(int mailId, const QString& label);
	Q_INVOKABLE void markMailStarred(int mailId, bool starred);
	Q_INVOKABLE void markMailImportant(int mailId, bool important);

signals:
	void connectionStateChanged();
	void busyChanged();
	void lastErrorChanged();
	void foldersChanged();
	void mailsChanged();
	void currentMailChanged();

	void connected();
	void disconnected();
	void mailSent();
	void errorOccurred(const QString& message);

	void mailsFetched();
	void mailFetched();
	void foldersFetched();
	void attachmentDownloaded(const QString& outputPath);

	void actionDone();

private:
	void onResult(MailResult result);
	void setBusy(bool value);
	void setLastError(const QString& value);

	CommandQueue m_queue;
	MailWorker m_worker;

	bool m_isConnected = false;
	bool m_isBusy = false;
	QString m_lastError;
	QVariantList m_folders;
	QVariantList m_mails;
	QVariantMap m_currentMail;
	QString m_lastDownloadedPath;
};
