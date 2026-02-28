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
    explicit SmtpSession(const std::string& domain);

    std::string Greeting() const;
    std::string ProcessLine(const std::string& line);
    bool IsClosed() const noexcept;

private:
    void ResetMessage();

private:
    std::string m_domain;
    SmtpState m_state{SmtpState::WAIT_HELO};

    std::string m_sender;
    std::vector<std::string> m_recipients;
    std::string m_body;
};
