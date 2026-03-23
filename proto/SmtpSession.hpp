#pragma once

#include <string>
#include <vector>

#include "SmtpCommand.hpp"
#include "MessageRepository.h"
#include "UserRepository.h"
#include "Base64Decoder.hpp"
#include "Base64Encoder.hpp"

enum class SmtpState
{
	WAIT_HELO,
	WAIT_MAIL,
	WAIT_RCPT,
	WAIT_DATA,
	RECEIVING_DATA,
	CLOSED,
	STARTTLS,
	AUTH_WAIT_USER,
	AUTH_WAIT_PASS
};

class SmtpSession
{
public:

    explicit SmtpSession(std::string domain,
			MessageRepository* message_repo,
			UserRepository* user_repo);

    std::string Greeting() const;

    std::string ProcessLine(const std::string& line);

    bool IsClosed() const noexcept;

	SmtpState getState() const noexcept { return m_state; }

    void SetSecure(bool secure) { m_secure = secure; }

    void ResetToHelo();

private:
	bool SaveMessage();

	static std::string ExtractUsername(const std::string& email);

    std::string HandleHelo(const SmtpCommand& command);

    std::string HandleMail(const SmtpCommand& command);

    std::string HandleRcpt(const SmtpCommand& command);

    std::string HandleData(const SmtpCommand& command);

    std::string HandleRset();

    std::string HandleNoop();

    std::string HandleQuit();

    std::string HandleStartTLS();

	std::string HandleAuth(const SmtpCommand& command);

	std::string HandleAuthLine(const std::string& line);

    void ResetMessage();

private:

    static constexpr size_t MAX_MESSAGE_SIZE =
        10 * 1024 * 1024;

    SmtpState m_state{SmtpState::WAIT_HELO};

    std::string m_domain;

    std::string m_sender;

    std::vector<std::string> m_recipients;

    std::string m_body;

    bool m_authenticated{false};

	bool m_secure{false};

	std::string m_auth_username; 

	MessageRepository* m_message_repo;

	UserRepository* m_user_repo;
};
