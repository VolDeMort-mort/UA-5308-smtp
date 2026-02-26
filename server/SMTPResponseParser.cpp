#include "SMTPResponseParser.h"


void SMTPResponseParser::parseLine(const std::vector<std::string>& lines, ServerResponse& resp) {
	resp.code = 0;
	resp.lines.clear();
	if (lines.empty()) return;

	const std::string& first = lines.front();
	if (first.size() >= 3 && isdigit(static_cast<unsigned char>(first[0])) &&
		isdigit(static_cast<unsigned char>(first[1])) &&
		isdigit(static_cast<unsigned char>(first[2]))) {
		resp.code = (first[0] - '0') * 100 + (first[1] - '0') * 10 + (first[2] - '0');
	}
	else {
		resp.code = 0;
	}

	for (const auto& ln : lines) {
		if (ln.size() > 4) {
			resp.lines.push_back(ln.substr(4));
		}
		else if (ln.size() == 4) {
			resp.lines.push_back(std::string());
		}
		else {
			resp.lines.push_back(ln);
		}
	}
}

bool SMTPResponseParser::try_parse_code(const std::string& line, int& out_code) {
	if (line.size() >= 3 &&
		std::isdigit(static_cast<unsigned char>(line[0])) &&
		std::isdigit(static_cast<unsigned char>(line[1])) &&
		std::isdigit(static_cast<unsigned char>(line[2]))) {
		out_code = (line[0] - '0') * 100 + (line[1] - '0') * 10 + (line[2] - '0');
		return true;
	}
	return false;
}

bool SMTPResponseParser::is_final_sep(const std::string& line) {
	if (line.size() >= 4) {
		char sep = line[3];
		return sep == ' ' || (sep != '-' && line.size() == 3);
	}
	return true;
}

