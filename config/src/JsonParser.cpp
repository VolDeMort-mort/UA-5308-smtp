#include "../include/JsonParser.h"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace SmtpClient {

// ────────────────────────────────────────────────────────────────
// Public static interface
// ────────────────────────────────────────────────────────────────

JsonParser::FlatMap JsonParser::Parse(const std::string& json)
{
	JsonParser parser(json);
	FlatMap    result;

	parser.SkipWhitespace();
	if (parser.m_pos < parser.m_json.size() && parser.m_json[parser.m_pos] == '{')
	{
		++parser.m_pos; // consume '{'
		parser.ParseObject("", result);
	}

	return result;
}

JsonParser::FlatMap JsonParser::ParseFile(const std::string& path)
{
	std::ifstream file(path);
	if (!file.is_open())
		return {};

	std::ostringstream ss;
	ss << file.rdbuf();
	return Parse(ss.str());
}

JsonParser::JsonParser(const std::string& json) : m_json(json), m_pos(0) {}

void JsonParser::SkipWhitespace()
{
	while (m_pos < m_json.size() &&
		   (m_json[m_pos] == ' ' || m_json[m_pos] == '\t' ||
		    m_json[m_pos] == '\r' || m_json[m_pos] == '\n'))
	{
		++m_pos;
	}
}

std::string JsonParser::ParseString()
{
	// Expects m_pos to be at the opening "
	++m_pos; // skip "
	std::string result;

	while (m_pos < m_json.size() && m_json[m_pos] != '"')
	{
		if (m_json[m_pos] == '\\' && m_pos + 1 < m_json.size())
		{
			++m_pos;
			switch (m_json[m_pos])
			{
			case '"':  result += '"';  break;
			case '\\': result += '\\'; break;
			case '/':  result += '/';  break;
			case 'n':  result += '\n'; break;
			case 'r':  result += '\r'; break;
			case 't':  result += '\t'; break;
			default:   result += m_json[m_pos]; break;
			}
		}
		else
		{
			result += m_json[m_pos];
		}
		++m_pos;
	}

	if (m_pos < m_json.size())
		++m_pos; // skip closing "

	return result;
}

std::string JsonParser::ParseNumber()
{
	std::size_t start = m_pos;
	if (m_pos < m_json.size() && m_json[m_pos] == '-')
		++m_pos;

	while (m_pos < m_json.size() &&
		   (std::isdigit(static_cast<unsigned char>(m_json[m_pos])) ||
		    m_json[m_pos] == '.' || m_json[m_pos] == 'e' ||
		    m_json[m_pos] == 'E' || m_json[m_pos] == '+' ||
		    m_json[m_pos] == '-'))
	{
		++m_pos;
	}

	return m_json.substr(start, m_pos - start);
}

std::string JsonParser::ParseLiteral()
{
	// Handles: true, false, null
	static const std::pair<std::string, std::string> literals[] = {
		{"true", "true"}, {"false", "false"}, {"null", "null"}};

	for (const auto& lit : literals)
	{
		if (m_json.compare(m_pos, lit.first.size(), lit.first) == 0)
			// 0 - words are equal
		{
			m_pos += lit.first.size();
			return lit.second;
		}
	}

	// Unknown token — skip to next delimiter
	std::size_t start = m_pos;
	while (m_pos < m_json.size() && m_json[m_pos] != ',' &&
		   m_json[m_pos] != '}' && m_json[m_pos] != ']')
	{
		++m_pos;
	}
	return m_json.substr(start, m_pos - start);
}

void JsonParser::ParseObject(const std::string& prefix, FlatMap& out)
{
	// Called after { has been consumed.
	SkipWhitespace();

	while (m_pos < m_json.size() && m_json[m_pos] != '}')
	{
		SkipWhitespace();

		if (m_json[m_pos] != '"')
			break; // incorrect JSON

		std::string key = ParseString();
		std::string full_key = prefix.empty() ? key : prefix + "." + key;
		// e.g. full_key = "Server.Port"

		SkipWhitespace();
		if (m_pos < m_json.size() && m_json[m_pos] == ':')
			++m_pos; // consume :

		ParseValue(full_key, out);

		SkipWhitespace();
		if (m_pos < m_json.size() && m_json[m_pos] == ',')
			++m_pos; // consume ,
	}

	if (m_pos < m_json.size() && m_json[m_pos] == '}')
		++m_pos; // consume }
}

void JsonParser::ParseValue(const std::string& key, FlatMap& out)
{
	SkipWhitespace();
	if (m_pos >= m_json.size())
		return;

	char c = m_json[m_pos];

	if (c == '{')
	{
		++m_pos; // consume {
		ParseObject(key, out);
	}
	else if (c == '"')
	{
		out[key] = ParseString();
	}
	else if (c == '-' || std::isdigit(static_cast<unsigned char>(c)))
	{
		out[key] = ParseNumber();
	}
	else
	{
		out[key] = ParseLiteral();
	}
}

} // namespace SmtpClient
