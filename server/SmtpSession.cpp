#include "SmtpSession.hpp"

#include "SmtpParser.hpp"
#include "SmtpResponse.hpp"

SmtpSession::SmtpSession(std::string domain)
    : m_domain(std::move(domain))
{
}

std::string SmtpSession::Greeting() const
{
    return SmtpResponse::ServiceReady(m_domain);
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

void SmtpSession::ResetToHelo()
{
	m_state = SmtpState::WAIT_HELO;
	ResetMessage();
}

std::string SmtpSession::ProcessLine(const std::string& line)
{
    constexpr size_t MAX_SMTP_LINE = 512;

    if (line.size() > MAX_SMTP_LINE)
        return SmtpResponse::SyntaxError();

    if (m_state == SmtpState::RECEIVING_DATA)
    {
        if (line == ".")
        {
            m_state = SmtpState::WAIT_MAIL;

            ResetMessage();

            return SmtpResponse::Ok();
        }

        m_body += line + "\n";

        if (m_body.size() > MAX_MESSAGE_SIZE)
        {
            m_state = SmtpState::WAIT_MAIL;

            ResetMessage();

            return SmtpResponse::MessageTooLarge();
        }

        return {};
    }

    SmtpCommand command = SmtpParser::Parse(line);

    switch (command.type)
    {
        case SmtpCommandType::HELO:
        case SmtpCommandType::EHLO:
            return HandleHelo(command);

        case SmtpCommandType::MAIL:
            return HandleMail(command);

        case SmtpCommandType::RCPT:
            return HandleRcpt(command);

        case SmtpCommandType::DATA:
            return HandleData(command);

        case SmtpCommandType::RSET:
            return HandleRset();

        case SmtpCommandType::NOOP:
            return HandleNoop();

        case SmtpCommandType::QUIT:
            return HandleQuit();

        case SmtpCommandType::UNKNOWN:
            return SmtpResponse::NotImplemented();

		case SmtpCommandType::STARTTLS:
			return HandleStartTLS();
    }

    return SmtpResponse::SyntaxError();
}

std::string SmtpSession::HandleHelo(const SmtpCommand& command)
{
    if (m_state != SmtpState::WAIT_HELO)
        return SmtpResponse::BadSequence();

    m_state = SmtpState::WAIT_MAIL;

	if (command.type == SmtpCommandType::EHLO)
	{
		return SmtpResponse::ReadyToStartTLS(m_domain);
	}

    return SmtpResponse::Hello(m_domain);
}

std::string SmtpSession::HandleMail(const SmtpCommand& command)
{
    if (m_state != SmtpState::WAIT_MAIL)
        return SmtpResponse::BadSequence();

    m_sender = command.argument;

    m_state = SmtpState::WAIT_RCPT;

    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleRcpt(const SmtpCommand& command)
{
    if (m_state != SmtpState::WAIT_RCPT)
        return SmtpResponse::BadSequence();

    m_recipients.push_back(command.argument);

    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleData(const SmtpCommand&)
{
    if (m_state != SmtpState::WAIT_RCPT ||
        m_recipients.empty())
    {
        return SmtpResponse::BadSequence();
    }

    m_state = SmtpState::RECEIVING_DATA;

    return SmtpResponse::StartMailInput();
}

std::string SmtpSession::HandleRset()
{
    ResetMessage();

    m_state = SmtpState::WAIT_MAIL;

    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleNoop()
{
    return SmtpResponse::Ok();
}

std::string SmtpSession::HandleQuit()
{
    m_state = SmtpState::CLOSED;

    return SmtpResponse::Closing();
}

std::string SmtpSession::HandleStartTLS()
{
	if (m_state != SmtpState::WAIT_MAIL)
	{
		return SmtpResponse::BadSequence();
	}

     m_state = SmtpState::STARTTLS;

     return SmtpResponse::StartTls();
}
