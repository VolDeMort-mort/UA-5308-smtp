#include "SmtpSession.hpp"

#include "SmtpParser.hpp"
#include "SmtpResponse.hpp"

SmtpSession::SmtpSession(std::string domain,
                         MessageRepository* message_repo,
                         UserRepository* user_repo)
    : m_domain(std::move(domain)),
      m_message_repo(message_repo),
      m_user_repo(user_repo)
{
}

std::string SmtpSession::ExtractUsername(const std::string& email)
{
    std::string clean = email;

    if (!clean.empty() && clean.front() == '<')
        clean.erase(0, 1);

    if (!clean.empty() && clean.back() == '>')
        clean.pop_back();

    auto pos = clean.find('@');

    if (pos == std::string::npos)
        return clean;

    return clean.substr(0, pos);
}

bool SmtpSession::SaveMessage()
{
    if (m_recipients.empty())
        return false;

    bool any_saved = false;

    for (const auto& recipient : m_recipients)
    {
        std::string username = ExtractUsername(recipient);

        auto user = m_user_repo->findByUsername(username);

        if (!user)
        {
            User new_user;
            new_user.username = username;
            new_user.password_hash = "smtp_auto";

            if (!m_user_repo->registerUser(new_user))
                continue;

            user = m_user_repo->findByUsername(username);
        }

        if (!user || !user->id)
            continue;

        Message msg;

        msg.user_id = user->id.value();
        msg.receiver = recipient;
        msg.body = m_body;
        msg.subject = "";
        msg.status = MessageStatus::Received;
        msg.is_seen = false;

        if (!m_message_repo->send(msg))
            continue;

        any_saved = true;
    }

    return any_saved;
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
			if(!SaveMessage())
			{
				ResetMessage();
				m_state = SmtpState::WAIT_MAIL;
				return SmtpResponse::MessageStorageFailed();
			}
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
