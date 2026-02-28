#include "SmtpParser.hpp"
#include "SmtpSession.hpp"

SmtpSession::SmtpSession(const std::string& domain)
    : m_domain(domain)
{
}

std::string SmtpSession::Greeting() const
{
    return "220 " + m_domain + " Service ready\r\n";
}

bool SmtpSession::IsClosed() const noexcept
{
    return m_state == SmtpState::CLOSED;
}

void SmtpSession::ResetMessage()
{
    m_sender.clear();
    m_recipients.clear();
    m_body.clear();
}

std::string SmtpSession::ProcessLine(const std::string& line)
{
    if (m_state == SmtpState::RECEIVING_DATA)
    {
        if (line == ".")
        {
            m_state = SmtpState::WAIT_MAIL;
            ResetMessage();
            return "250 OK\r\n";
        }

        m_body += line + "\n";
        return {};
    }

    SmtpCommand command = SmtpParser::Parse(line);

    switch (command.type)
    {
        case SmtpCommandType::HELO:
        case SmtpCommandType::EHLO:
            if (m_state != SmtpState::WAIT_HELO)
                return "503 Bad sequence of commands\r\n";

            m_state = SmtpState::WAIT_MAIL;
            return "250 " + m_domain + " greets you\r\n";

        case SmtpCommandType::MAIL:
            if (m_state != SmtpState::WAIT_MAIL)
                return "503 Bad sequence of commands\r\n";

            m_sender = command.argument;
            m_state = SmtpState::WAIT_RCPT;
            return "250 OK\r\n";

        case SmtpCommandType::RCPT:
            if (m_state != SmtpState::WAIT_RCPT)
                return "503 Bad sequence of commands\r\n";

            m_recipients.push_back(command.argument);
            return "250 OK\r\n";

        case SmtpCommandType::DATA:
            if (m_state != SmtpState::WAIT_RCPT || m_recipients.empty())
                return "503 Bad sequence of commands\r\n";

            m_state = SmtpState::RECEIVING_DATA;
            return "354 End data with <CR><LF>.<CR><LF>\r\n";

        case SmtpCommandType::RSET:
            ResetMessage();
            m_state = SmtpState::WAIT_MAIL;
            return "250 OK\r\n";

        case SmtpCommandType::NOOP:
            return "250 OK\r\n";

        case SmtpCommandType::QUIT:
            m_state = SmtpState::CLOSED;
            return "221 Bye\r\n";

        case SmtpCommandType::UNKNOWN:
            return "502 Command not implemented\r\n";
    }

    return "500 Syntax error\r\n";
}
