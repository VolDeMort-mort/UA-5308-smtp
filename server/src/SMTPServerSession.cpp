#include <algorithm>
#include <cctype>
#include <iostream>

#include "SMTPServerSession.h"

// no logs for now

using boost::system::error_code;

SMTPServerSession::SMTPServerSession(SMTPServerSocket::Ptr socket, std::shared_ptr<boost::asio::ssl::context> ssl_ctx)
	: m_socket(std::move(socket)), m_ssl_ctx(std::move(ssl_ctx)), m_state(State::NEW)
{
}

void SMTPServerSession::run()
{
	ServerResponse greeting{220, {"SMTP Server ready"}};

	auto self = shared_from_this();
	m_socket->SendResponse(
		greeting,
		[self](const error_code& ec)
		{
			if (ec)
			{
				self->m_state = State::CLOSED;
				return;
			}
			self->m_state = State::NEW;
			self->DoReadCommand();
		});
}

void SMTPServerSession::SendResponse(const ServerResponse& resp)
{
	auto self = shared_from_this();
	m_socket->SendResponse(
		resp,
		[self, resp](const error_code& ec)
		{
			if (ec)
			{
				self->m_state = State::CLOSED;
				return;
			}

			self->DoReadCommand();
		});
}

void SMTPServerSession::DoReadCommand()
{
	if (m_state == State::CLOSED) return;

	auto self = shared_from_this();

	m_socket->ReadCommand(M_CMD_TIMEOUT,
						  [self](ClientCommand cmd, const error_code& ec) { self->HandleCommand(std::move(cmd), ec); });
}

void SMTPServerSession::HandleCommand(ClientCommand cmd, const error_code& ec)
{
	if (ec)
	{
		m_state = State::CLOSED;
		m_socket->Close();
		return;
	}

	if (cmd.command == CommandType::QUIT)
	{
		ServerResponse resp{221, {"Bye"}};
		auto self = shared_from_this();

		m_socket->SendResponse(
			resp,
			[self](const error_code& ec)
			{
				self->m_state = State::CLOSED;
				self->m_socket->Close();
				return;
			});
		return;
	}

	if (m_state == State::READING_DATA)
	{
		ServerResponse resp{503, {"Bad sequence of commands"}};
		SendResponse(resp);
		return;
	}

	switch (cmd.command)
	{
	case CommandType::HELO:
	case CommandType::EHLO:
	{
		m_mail_from.clear();
		m_recipients.clear();
		m_data_buffer.clear();

		ServerResponse resp;
		resp.code = 250;
		resp.lines = {"Hello"};

		if (cmd.command == CommandType::EHLO)
		{
			if (m_ssl_ctx && !m_socket->IsTls()) resp.lines.push_back("STARTTLS");
		}

		m_state = State::READY;
		SendResponse(resp);
		break;
	}
	case CommandType::MAIL:
	{
		if (m_state != State::READY)
		{
			ServerResponse resp{503, {"Bad sequence of commands: send HELO/EHLO first"}};
			SendResponse(resp);
			break;
		}

		std::string addr = cmd.commandText;
		if (!ExtractAddress(addr))
		{
			ServerResponse resp{501, {"Syntax error in parameters or arguments"}};
			SendResponse(resp);
		}
		else
		{
			m_mail_from = addr;
			m_recipients.clear();
			m_data_buffer.clear();
			ServerResponse resp{250, {"Sender OK"}};
			SendResponse(resp);
		}
		break;
	}
	case CommandType::RCPT:
	{
		if (m_mail_from.empty())
		{
			ServerResponse resp{503, {"Bad sequence of commands: MAIL required before RCPT"}};
			SendResponse(resp);
			break;
		}

		std::string addr = cmd.commandText;
		if (!ExtractAddress(addr))
		{
			ServerResponse resp{501, {"Syntax error in parameters or arguments"}};
			SendResponse(resp);
		}
		else
		{
			m_recipients.push_back(addr);
			ServerResponse resp{250, {"Recipient OK"}};
			SendResponse(resp);
		}
		break;
	}
	case CommandType::DATA:
	{
		if (m_mail_from.empty() || m_recipients.empty())
		{
			ServerResponse resp{503, {"Bad sequence of commands"}};
			SendResponse(resp);
		}
		else
		{
			ServerResponse resp{354, {"End data with <CR><LF>.<CR><LF>"}};
			auto self = shared_from_this();
			m_socket->SendResponse(
				resp,
				[self](const error_code& ec)
				{
					if (ec)
					{
						self->m_state = State::CLOSED;
						self->m_socket->Close();
						return;
					}
					self->StartReadData();
				});
		}
		break;
	}
	case CommandType::STARTTLS:
	{

		if (!m_ssl_ctx)
		{
			SendResponse({454, {"TLS not available"}});
			break;
		}

		if (m_socket->IsTls())
		{
			SendResponse({503, {"Already using TLS"}});
			break;
		}

		auto self = shared_from_this();

		m_socket->SendResponse(
			{220, {"Ready to start TLS d"}},
			[self](const error_code& ec)
			{
				if (ec)
				{
					self->m_state = State::CLOSED;
					self->m_socket->Close();
					return;
				}

				self->m_socket->StartTls(
					*self->m_ssl_ctx,
					[self](const error_code& ec2)
					{
						if (ec2)
						{
							self->m_state = State::CLOSED;
							self->m_socket->Close();
							return;
						}
						self->m_mail_from.clear();
						self->m_recipients.clear();
						self->m_data_buffer.clear();
						self->m_state = State::NEW;

						self->DoReadCommand();
					});
			});

		break;
	}
	default:
	{
		ServerResponse resp{500, {"Command unrecognized"}};
		SendResponse(resp);
		break;
	}
	}
}

void SMTPServerSession::StartReadData()
{
	if (m_state == State::CLOSED) return;
	m_state = State::READING_DATA;
	auto self = shared_from_this();
	m_socket->ReadDataBlock(
		M_DATA_TIMEOUT,
		[self](ClientCommand cmd, const boost::system::error_code& ec)
		{
			if (ec)
			{
				self->m_state = State::CLOSED;
				self->m_socket->Close();
				return;
			}

			if (cmd.command != CommandType::SEND_DATA)
			{
				ServerResponse resp{500, {"Unexpected data"}};
				self->SendResponse(resp);
				return;
			}

			self->m_data_buffer = std::move(cmd.commandText);

			// do something usefull with recived data

			ServerResponse resp{250, {"OK: message accepted"}};

			self->m_mail_from.clear();
			self->m_recipients.clear();
			self->m_data_buffer.clear();
			self->m_state = State::READY;
			self->SendResponse(resp);
		});
}

bool SMTPServerSession::ExtractAddress(std::string& s)
{
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
		s.erase(s.begin());

	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
		s.pop_back();

	if (s.empty()) return false;

	if (s.front() == '<') s.erase(s.begin());

	if (!s.empty() && s.back() == '>') s.pop_back();

	return !s.empty();
}
