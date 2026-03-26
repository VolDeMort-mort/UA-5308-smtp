#pragma once
#include <string>
#include <unordered_map>

namespace SmtpClient {

class JsonParser
{
public:
	using FlatMap = std::unordered_map<std::string, std::string>;

	static FlatMap Parse(const std::string& json);

	static FlatMap ParseFile(const std::string& path);

private:
	explicit JsonParser(const std::string& json);

	void SkipWhitespace();
	std::string ParseString();
	std::string ParseNumber();
	std::string ParseLiteral(); // for keywords 
	void ParseObject(const std::string& prefix, FlatMap& out);
	void ParseValue(const std::string& key, FlatMap& out);

	const std::string& m_json; // entire file
	std::size_t m_pos = 0;
};

} // namespace SmtpClient
