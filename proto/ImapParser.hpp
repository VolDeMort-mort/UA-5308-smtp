#pragma once

#include <string>
#include <vector>

#include "ImapCommand.hpp"

class ImapParser
{
public:
	static ImapCommand Parse(const std::string& line);

private:
	class ElementParser
	{
	public:
		explicit ElementParser(const std::string& str) : m_str(str), m_pos(0) {}

		std::vector<std::string> ParseArgs();

	private:
		std::string m_str;
		size_t m_pos;

		void SkipWhitespace();
		std::string ParseElement();
		std::string ParseQuoted();
		std::string ParseList();
		std::string ParseResponse();
		std::string ParseLiteral();
		std::string ParseAtom();
	};

	static std::vector<std::string> ParseArguments(const std::string& args);
};
