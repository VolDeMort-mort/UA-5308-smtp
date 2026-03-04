#pragma once

#include <functional>
#include <memory>
#include <thread>

#include <boost/asio.hpp>

#include "CommandQueue.hpp"
#include "MailResult.hpp"

// Forward declarations to avoid pulling Boost into headers
class SocketConnector;
class SocketConnection;
class SmtpSession;

class MailWorker
{
public:
	// onResult is called from the worker thread when a command completes;
	// Bridge subscribes to this callback to receive results.
	explicit MailWorker(CommandQueue& queue, std::function<void(MailResult)> onResult);
	~MailWorker();

	void start(); // start the worker thread
	void stop();  // stop the worker thread (blocks until done)

private:
	void run(); // main loop running inside the worker thread

	// command handlers
	void handleConnect(const ConnectCommand& cmd);
	void handleDisconnect(const DisconnectCommand& cmd);
	void handleSendMail(const SendMailCommand& cmd);
	void handleFetchMails(const FetchMailsCommand& cmd);
	void handleFetchMail(const FetchMailCommand& cmd);
	void handleDeleteMail(const DeleteMailCommand& cmd);
	void handleMarkMailRead(const MarkMailReadCommand& cmd);
	void handleFetchFolders(const FetchFoldersCommand& cmd);
	void handleCreateFolder(const CreateFolderCommand& cmd);
	void handleDeleteFolder(const DeleteFolderCommand& cmd);
	void handleAddLabel(const AddLabelCommand& cmd);
	void handleRemoveLabel(const RemoveLabelCommand& cmd);
	void handleMarkMailStarred(const MarkMailStarredCommand& cmd);
	void handleMarkMailImportant(const MarkMailImportantCommand& cmd);

	// IMAP helpers ----------------------------------------------------
	// Generate next unique IMAP tag: A1, A2, A3...
	std::string nextTag();

	// Send one IMAP command and read all lines until the tagged response.
	// Returns { success, untaggedLines } where untaggedLines are all
	// "* ..." lines received before the final tagged response.
	std::pair<bool, std::vector<std::string>> imapExchange(const std::string& cmd);

	CommandQueue& m_queue;
	std::function<void(MailResult)> m_onResult;
	std::thread m_thread;

	// io_context lives as long as Worker — sockets reference it
	boost::asio::io_context m_ioContext;

	// SMTP state
	std::unique_ptr<SocketConnector> m_connector;
	std::unique_ptr<SocketConnection> m_smtpConnection;
	std::unique_ptr<SmtpSession> m_smtpSession;

	// IMAP state
	std::unique_ptr<SocketConnection> m_imapConnection;
	uint32_t m_tagCounter{0};          // IMAP tag counter
	std::string m_currentFolder;       // last selected folder (SELECT)

	// Saved connection data — needed for SMTP reconnect
	std::string m_smtpHost;
	uint16_t    m_smtpPort{0};
};
