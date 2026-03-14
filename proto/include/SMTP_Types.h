#pragma once

#include <vector>
#include <string>

constexpr size_t MAX_SMTP_LINE = 1024;
constexpr size_t MAX_DATA_SIZE = 10 * 1024 * 1024;

enum CommandType {
	HELO,
	EHLO,
	MAIL,
	RCPT,
	DATA,
	QUIT,
	SEND_DATA,
	STARTTLS,
	UNKNOWN
};

struct ClientCommand {
	ClientCommand() = default;
	ClientCommand(CommandType type, std::string text)
		: command(type), commandText(std::move(text))
	{}
	CommandType command = CommandType::UNKNOWN;
	std::string commandText;
};


struct ServerResponse {
	ServerResponse() = default;
	ServerResponse(int nCode, std::vector<std::string> text)
		: code(nCode), lines(std::move(text))
	{}
	int code = 0;
	std::vector<std::string> lines;
};