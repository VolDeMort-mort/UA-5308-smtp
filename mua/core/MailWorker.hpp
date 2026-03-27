#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <thread>

#include <boost/asio.hpp>

#include "CommandQueue.hpp"
#include "MailResult.hpp"

class SocketConnector;
class SocketConnection;

class MailWorker
{
public:
	explicit MailWorker(CommandQueue& queue, std::function<void(MailResult)> onResult);
	~MailWorker();

	void start();
	void stop();

private:
	void run();

	void handleConnect(const ConnectCommand& cmd);
	void handleDisconnect(const DisconnectCommand& cmd);
	void handleSendMail(const SendMailCommand& cmd);
	void handleFetchMails(const FetchMailsCommand& cmd);
	void handleFetchMail(const FetchMailCommand& cmd);
	void handleDeleteMail(const DeleteMailCommand& cmd);
	void handleMarkMailRead(const MarkMailReadCommand& cmd);
	void handleMoveMail(const MoveMailCommand& cmd);
	void handleDownloadAttachment(const DownloadAttachmentCommand& cmd);
	void handleFetchFolders(const FetchFoldersCommand& cmd);
	void handleCreateFolder(const CreateFolderCommand& cmd);
	void handleDeleteFolder(const DeleteFolderCommand& cmd);
	void handleAddLabel(const AddLabelCommand& cmd);
	void handleRemoveLabel(const RemoveLabelCommand& cmd);
	void handleMarkMailStarred(const MarkMailStarredCommand& cmd);
	void handleMarkMailImportant(const MarkMailImportantCommand& cmd);

	bool authenticateSmtp(SocketConnection& connection,
	                     const std::string& username,
	                     const std::string& password,
	                     std::string& error);
	bool loginImap(const std::string& host,
	               uint16_t port,
	               const std::string& username,
	               const std::string& password,
	               std::string& error);
	bool reconnectSession(std::string& error);
	bool ensureImapConnected(std::string& error);
	void reportReconnectState(bool reconnecting, uint32_t attempt, const std::string& message);

	std::string nextTag();

	std::pair<bool, std::vector<std::string>> imapExchange(const std::string& cmd);

	CommandQueue& m_queue;
	std::function<void(MailResult)> m_onResult;
	std::thread m_thread;

	boost::asio::io_context m_ioContext;

	std::unique_ptr<SocketConnector> m_connector;
	std::unique_ptr<SocketConnection> m_smtpConnection;

	std::unique_ptr<SocketConnection> m_imapConnection;
	uint32_t m_tagCounter{0};         
	std::string m_currentFolder;

	std::string m_smtpHost;
	uint16_t    m_smtpPort{0};
	std::string m_smtpUsername;
	std::string m_smtpPassword;

	std::string m_imapHost;
	uint16_t    m_imapPort{0};
	std::string m_imapUsername;
	std::string m_imapPassword;
	std::atomic<bool> m_stopRequested{false};
};
