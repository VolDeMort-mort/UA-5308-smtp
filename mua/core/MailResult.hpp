#pragma once

#include <cstdint>
#include <string>
#include <variant>
#include <vector>

struct MailActionResult
{
	bool success;
	std::string error;
};

// ConnectCommand
struct ConnectResult
{
	bool success;
	std::string error;
};

// DisconnectCommand
struct DisconnectResult
{
	bool success;
	std::string error;
};

struct SendMailResult
{
	bool success;
	std::string error;
};

struct MailSummary
{
	int64_t mailId = 0;
	bool isSeen = false;
	bool isFlagged = false;
	std::string subject;
	//std::string preview;
	std::string date;
};

struct AttachmentInfo
{
	std::string fileName;
	std::string mimeType;
	std::size_t sizeBytes = 0;
};

struct MailDetails
{
	int64_t mailId = 0;
	bool isSeen = false;
	bool isFlagged = false;
	std::string sender;
	std::vector<std::string> to;
	std::vector<std::string> cc;
	std::vector<std::string> bcc;
	std::vector<std::string> replyTo;
	std::string subject;
	std::string date;
	std::string messageId;
	std::string inReplyTo;
	std::string references;
	std::string plainBody;
	std::string htmlBody;
	std::string body;
	std::vector<AttachmentInfo> attachments;
};

struct FetchMailsResult
{
	bool success;
	std::string error;
	std::vector<MailSummary> mails;
};

struct FetchMailResult
{
	bool success;
	std::string error;
	MailDetails mail;
};

struct FetchFoldersResult
{
	bool success;
	std::string error;
	std::vector<std::string> folders;
};

struct DownloadAttachmentResult
{
	bool success;
	std::string error;
	std::string outputPath;
	std::string fileName;
	std::size_t bytesWritten;
};

struct ReconnectStateResult
{
	bool reconnecting = false;
	uint32_t attempt = 0;
	std::string message;
};

using MailResult = std::variant<
    ConnectResult,
    DisconnectResult,
    SendMailResult,
    FetchMailsResult,
    FetchMailResult,
    FetchFoldersResult,
    DownloadAttachmentResult,
	ReconnectStateResult,
    MailActionResult>;
