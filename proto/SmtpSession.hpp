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

    std::string Greeting() const;

    std::string ProcessLine(const std::string& line);

    bool IsClosed() const noexcept;

private:

    std::string HandleHelo(const SmtpCommand& command);

    std::string HandleMail(const SmtpCommand& command);

    std::string HandleRcpt(const SmtpCommand& command);

    std::string HandleData(const SmtpCommand& command);

    std::string HandleRset();

    std::string HandleNoop();

    std::string HandleQuit();

    void ResetMessage();

private:

    static constexpr size_t MAX_MESSAGE_SIZE =
        10 * 1024 * 1024;

    SmtpState m_state{SmtpState::WAIT_HELO};

    std::string m_domain;

    std::string m_sender;

    std::vector<std::string> m_recipients;

    std::string m_body;
};
