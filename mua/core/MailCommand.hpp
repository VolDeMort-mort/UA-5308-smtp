#pragma once
#include <string>
#include <variant>
#include <vector>
#include <cstdint>

// ID type matches database/Entity/Message.h (int64_t)

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
	std::vector<std::string> recipients;
	std::string subject;
	std::string body;
	std::vector<std::string> attachments;
};

struct FetchMailsCommand
{
	std::string folderName; // IMAP uses folder names, not IDs
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

struct FetchFoldersCommand
{
};

struct CreateFolderCommand
{
	std::string folderName;
};

struct DeleteFolderCommand
{
	std::string folderName; // IMAP deletes by folder name
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
    FetchFoldersCommand,
    CreateFolderCommand,
    DeleteFolderCommand,
    AddLabelCommand,
    RemoveLabelCommand,
    MarkMailStarredCommand,
    MarkMailImportantCommand>;