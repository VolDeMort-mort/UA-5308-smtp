#pragma once
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

struct ConnectCommand
{
	std::string SMTPhost;
	uint16_t SMTPport;

	std::string IMAPhost;
	uint16_t IMAPport;

	std::string username;
	std::string password;
};

struct DisconnectCommand
{
};

struct SendMailCommand
{
	std::string sender;
	std::vector<std::string> to;
	std::vector<std::string> cc;
	std::vector<std::string> bcc;
	std::vector<std::string> recipients;
	std::string subject;
	std::string body;
	std::string htmlBody;
	std::vector<std::string> attachments;
};

struct FetchMailsCommand
{
	std::string folderName;
	unsigned int offset;
	unsigned int limit;
};

struct FetchMailCommand
{
	int64_t mailId;
};

struct DeleteMailCommand
{
	int64_t mailId;
};

struct MarkMailReadCommand
{
	int64_t mailId;
};

struct MoveMailCommand
{
	int64_t mailId;
	std::string targetFolder;
};

struct DownloadAttachmentCommand
{
	int64_t mailId;
	std::size_t attachmentIndex;
	std::string outputPath;
};

struct FetchFoldersCommand
{
};

struct CreateFolderCommand
{
	std::string folderName;
};

struct DeleteFolderCommand
{
	std::string folderName;
};

struct AddLabelCommand
{
	int64_t mailId;
	std::string label;
};

struct RemoveLabelCommand
{
	int64_t mailId;
	std::string label;
};

struct MarkMailStarredCommand
{
	int64_t mailId;
	bool starred;
};

struct MarkMailImportantCommand
{
	int64_t mailId;
	bool important;
};

using MailCommand = std::variant<
    ConnectCommand,
    DisconnectCommand,
    SendMailCommand,
    FetchMailsCommand,
    FetchMailCommand,
    DeleteMailCommand,
    MarkMailReadCommand,
	MoveMailCommand,
	DownloadAttachmentCommand,
    FetchFoldersCommand,
    CreateFolderCommand,
    DeleteFolderCommand,
    AddLabelCommand,
    RemoveLabelCommand,
    MarkMailStarredCommand,
    MarkMailImportantCommand>;
