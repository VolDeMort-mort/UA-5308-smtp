#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>

#include "SMTPCommandParser.h"

static void trim(std::string& s)
{
	size_t start = 0;
	while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
	size_t end = s.size();
	while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
	s = s.substr(start, end - start);
}

ClientCommand SMTPCommandParser::ParseLine(std::string_view line)
{
	ClientCommand cmd;
	cmd.command = CommandType::UNKNOWN;

	if (!line.empty() && line.back() == '\n') line.remove_suffix(1);
	if (!line.empty() && line.back() == '\r') line.remove_suffix(1);

	size_t pos = line.find(' ');
	std::string_view verb = (pos == std::string_view::npos) ? line : line.substr(0, pos);
	std::string_view rest = (pos == std::string_view::npos) ? std::string_view{} : line.substr(pos + 1);

	std::string verb_up;
	verb_up.reserve(verb.size());
	for (char c : verb)
		verb_up += std::toupper(static_cast<unsigned char>(c));

	if (verb_up == "MAIL" || verb_up == "RCPT")
	{
		size_t colon = rest.find(':');
		std::string_view key = (colon == std::string_view::npos) ? rest : rest.substr(0, colon);
		std::string_view value = (colon == std::string_view::npos) ? std::string_view{} : rest.substr(colon + 1);

		std::string key_up;
		key_up.reserve(key.size());
		for (char c : key)
			key_up += std::toupper(static_cast<unsigned char>(c));

		std::string expected = (verb_up == "MAIL") ? "FROM" : "TO";
		if (key_up != expected) return cmd;

		std::string address(value);
		trim(address);
		if (!address.empty() && address.front() == '<' && address.back() == '>')
			address = address.substr(1, address.size() - 2);

		if (!address.empty())
		{
			cmd.command = (verb_up == "MAIL") ? CommandType::MAIL : CommandType::RCPT;
			cmd.commandText = std::move(address);
		}
		return cmd;
	}

	if (verb_up == "HELO")
		cmd.command = CommandType::HELO;
	else if (verb_up == "EHLO")
		cmd.command = CommandType::EHLO;
	else if (verb_up == "DATA")
		cmd.command = CommandType::DATA;
	else if (verb_up == "STARTTLS")
		cmd.command = CommandType::STARTTLS;
	else if (verb_up == "QUIT")
		cmd.command = CommandType::QUIT;

	std::string text(rest);
	trim(text);
	cmd.commandText = std::move(text);

	return cmd;
}

ClientCommand SMTPCommandParser::ParseData(std::string_view data) {
	const std::string term = "\r\n.\r\n";
	if (data.size() >= 5 && data.substr(data.size() - 5) == "\r\n.\r\n")
	{
		data.remove_suffix(5);
	}
	else if (data.size() >= 3 && data.substr(data.size() - 3) == ".\r\n")
	{
		data.remove_suffix(3);
	}

	std::string out;
	out.reserve(data.size());

	size_t pos = 0;
	while (pos < data.size())
	{
		size_t lf = data.find('\n', pos);
		std::string_view line;
		if (lf == std::string::npos)
		{
			line = std::string_view(data.data() + pos, data.size() - pos);
			pos = data.size();
		}
		else
		{
			line = std::string_view(data.data() + pos, lf - pos);
			pos = lf + 1;
		}

		if (!line.empty() && line.back() == '\r')
		{
			line = std::string_view(line.data(), line.size() - 1);
		}

		if (line.size() >= 2 && line[0] == '.' && line[1] == '.')
		{
			out.append(line.data() + 1, line.size() - 1);
		}
		else
		{
			out.append(line.data(), line.size());
		}
		out += "\r\n";
	}

	ClientCommand cmd(CommandType::SEND_DATA, std::move(out));
	return cmd;
}