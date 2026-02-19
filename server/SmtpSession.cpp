#include "SmtpSession.hpp"
#include "SmtpParser.hpp"
#include "SmtpResponse.hpp"

SmtpSession::SmtpSession(std::string domain)
    : m_domain(std::move(domain))
{}

std::string SmtpSession::greeting() const
{
    return SmtpResponse::serviceReady(m_domain);
}

bool SmtpSession::isClosed() const noexcept
{
    return m_state == SmtpState::CLOSED;
}

void SmtpSession::resetMessage()
{
    m_sender.clear();
    m_recipients.clear();
    m_body.clear();
}

std::string SmtpSession::processLine(const std::string& line)
{
    // DATA mode
    if (m_state == SmtpState::RECEIVING_DATA)
    {
        if (line == ".") {
            m_state = SmtpState::WAIT_MAIL;
            resetMessage();
            return SmtpResponse::ok();
        }

        m_body += line + "\n";
        return {};
    }

    auto cmd = SmtpParser::parse(line);

    switch (cmd.type)
    {
        case SmtpCommandType::HELO:
        case SmtpCommandType::EHLO:
            if (m_state != SmtpState::WAIT_HELO)
                return SmtpResponse::badSequence();

            m_state = SmtpState::WAIT_MAIL;
            return SmtpResponse::hello(m_domain);

        case SmtpCommandType::MAIL:
            if (m_state != SmtpState::WAIT_MAIL)
                return SmtpResponse::badSequence();

            m_sender = cmd.argument;
            m_state = SmtpState::WAIT_RCPT;
            return SmtpResponse::ok();

        case SmtpCommandType::RCPT:
            if (m_state != SmtpState::WAIT_RCPT)
                return SmtpResponse::badSequence();

            m_recipients.push_back(cmd.argument);
            return SmtpResponse::ok();

        case SmtpCommandType::DATA:
            if (m_state != SmtpState::WAIT_RCPT ||
                m_recipients.empty())
                return SmtpResponse::badSequence();

            m_state = SmtpState::RECEIVING_DATA;
            return SmtpResponse::startMailInput();

        case SmtpCommandType::RSET:
            resetMessage();
            m_state = SmtpState::WAIT_MAIL;
            return SmtpResponse::ok();

        case SmtpCommandType::NOOP:
            return SmtpResponse::ok();

        case SmtpCommandType::QUIT:
            m_state = SmtpState::CLOSED;
            return SmtpResponse::closing();

        case SmtpCommandType::UNKNOWN:
            return SmtpResponse::commandNotImplemented();
    }

    return SmtpResponse::syntaxError();
}

