#pragma once
#include <string>
#include <vector>

#include "SmtpCommand.hpp"

enum class SmtpState
{
	WAIT_HELO,
	WAIT_MAIL,
	WAIT_RCPT,
	WAIT_DATA,
	RECEIVING_DATA,
	CLOSED
};

class SmtpSession
{
public:
	explicit SmtpSession(std::string domain);

	std::string greeting() const;

	std::string processLine(const std::string& line);

	bool isClosed() const noexcept;

private:
	void resetMessage();

	SmtpState m_state{SmtpState::WAIT_HELO};
	std::string m_domain;

	std::string m_sender;
	std::vector<std::string> m_recipients;
	std::string m_body;
};
