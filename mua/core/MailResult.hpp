#pragma once

#include <string>
#include <variant>
#include <vector>

#include "MailData.hpp"

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

// SendMailCommand
struct SendMailResult
{
	bool success;
	std::string error;
};

struct FetchMailsResult
{
	bool success;
	std::string error;
	std::vector<Message> mails;
};

struct FetchMailResult
{
	bool success;
	std::string error;
	Message mail;
};

struct FetchFoldersResult
{
	bool success;
	std::string error;
	std::vector<Folder> folders;
};

using MailResult = std::variant<
    ConnectResult,
    DisconnectResult,
    SendMailResult,
    FetchMailsResult,
    FetchMailResult,
    FetchFoldersResult,
    MailActionResult>;
