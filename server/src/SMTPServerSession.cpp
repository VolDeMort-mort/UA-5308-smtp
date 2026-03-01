#include "SMTPServerSession.h"
#include <iostream>
#include <algorithm>
#include <cctype>

// no logs for now

using boost::system::error_code;

SMTPServerSession::SMTPServerSession(SMTPServerSocket::Ptr socket, std::shared_ptr<boost::asio::ssl::context> ssl_ctx)
	: m_socket(std::move(socket)), m_ssl_ctx(std::move(ssl_ctx)), m_state(State::NEW)
{}

void SMTPServerSession::run() {
	ServerResponse greeting{220, {"SMTP Server ready"}};

	auto self = shared_from_this();
	m_socket->sendResponse(greeting, [self](const error_code& ec) {
		if (ec) {
			self->m_state = State::CLOSED;
			return;
		}
		self->m_state = State::NEW;
		self->do_read_command();
		});
}

void SMTPServerSession::send_response(const ServerResponse& resp) {
	auto self = shared_from_this();
	m_socket->sendResponse(resp, [self, resp](const error_code& ec) {
		if (ec) {
			self->m_state = State::CLOSED;
			return;
		}

		self->do_read_command();
		});
}

void SMTPServerSession::do_read_command() {
	if (m_state == State::CLOSED) return;

	auto self = shared_from_this();

	m_socket->readCommand(m_cmd_timeout, [self](ClientCommand cmd, const error_code& ec) {
		self->handle_command(std::move(cmd), ec);
		});
}

void SMTPServerSession::handle_command(ClientCommand cmd, const error_code& ec) {
	if (ec) {
		m_state = State::CLOSED;
		m_socket->close();
		return;
	}

	if (cmd.command == CommandType::QUIT) {
		ServerResponse resp{ 221, { "Bye" } };
		auto self = shared_from_this();

		m_socket->sendResponse(resp, [self](const error_code& ec) {
			self->m_state = State::CLOSED;
			self->m_socket->close();
			return;
			});
	}

	if (m_state == State::READING_DATA) {
		ServerResponse resp{ 503, { "Bad sequence of commands" } };
		send_response(resp);
		return;
	}

	switch (cmd.command) {
	case CommandType::HELO:
	case CommandType::EHLO: {
		m_mail_from.clear();
		m_recipients.clear();
		m_data_buffer.clear();

		ServerResponse resp;
		resp.code = 250;
		resp.lines = { "Hello"};

		if (m_ssl_ctx && !m_socket->is_tls()) resp.lines.push_back("STARTTLS");

		m_state = State::READY;
		send_response(resp);
		break;
	}
	case CommandType::MAIL: {
		if (m_state != State::READY) {
			ServerResponse resp{ 503, { "Bad sequence of commands: send HELO/EHLO first" } };
			send_response(resp);
			break;
		}

		std::string addr = cmd.commandText;
		if (!extract_address(addr)) {
			ServerResponse resp{ 501, { "Syntax error in parameters or arguments" } };
			send_response(resp);
		}
		else {
			m_mail_from = addr;
			m_recipients.clear();
			m_data_buffer.clear();
			ServerResponse resp{ 250, { "Sender OK" } };
			send_response(resp);
		}
		break;
	}
	case CommandType::RCPT: {
		if (m_mail_from.empty()) {
			ServerResponse resp{ 503, { "Bad sequence of commands: MAIL required before RCPT" } };
			send_response(resp);
			break;
		}

		std::string addr = cmd.commandText;
		if (!extract_address(addr)) {
			ServerResponse resp{ 501, { "Syntax error in parameters or arguments" } };
			send_response(resp);
		}
		else {
			m_recipients.push_back(addr);
			ServerResponse resp{ 250, { "Recipient OK" } };
			send_response(resp);
		}
		break;
	}
	case CommandType::DATA: {
		if (m_mail_from.empty() || m_recipients.empty()) {
			ServerResponse resp{ 503, { "Bad sequence of commands" } };
			send_response(resp);
		}
		else {
			ServerResponse resp{ 354, { "End data with <CR><LF>.<CR><LF>" } };
			auto self = shared_from_this();
			m_socket->sendResponse(resp, [self](const error_code& ec) {
				if (ec) {
					self->m_state = State::CLOSED;
					self->m_socket->close();
					return;
				}
				self->start_read_data();
				});
		}
		break;
	}
	case CommandType::STARTTLS:
	{

		if (!m_ssl_ctx){
			send_response({454, {"TLS not available"}});
			break;
		}

		if (m_socket->is_tls()){
			send_response({503, {"Already using TLS"}});
			break;
		}

		auto self = shared_from_this();

		m_socket->sendResponse({220, {"Ready to start TLS"}},
			[self](const error_code& ec){
				if (ec){
					self->m_state = State::CLOSED;
					self->m_socket->close();
					return;
				}
				
				self->m_socket->start_tls(*self->m_ssl_ctx,
					[self](const error_code& ec2){
						if (ec2){
							self->m_state = State::CLOSED;
							self->m_socket->close();
							return;
						}
						self->m_mail_from.clear();
						self->m_recipients.clear();
						self->m_data_buffer.clear();
						self->m_state = State::NEW;

						self->do_read_command();
					});
			});

		break;
	}
	default: {
		ServerResponse resp{ 500, { "Command unrecognized" } };
		send_response(resp);
		break;
	}
	}
}

void SMTPServerSession::start_read_data() {
	if (m_state == State::CLOSED) return;
	m_state = State::READING_DATA;
	auto self = shared_from_this();
	m_socket->readDataBlock(m_data_timeout, [self](ClientCommand cmd, const boost::system::error_code& ec) {
		if (ec) {
			self->m_state = State::CLOSED;
			self->m_socket->close();
			return;
		}

		if (cmd.command != CommandType::SEND_DATA) {
			ServerResponse resp{ 500, { "Unexpected data" } };
			self->send_response(resp);
			return;
		}

		self->m_data_buffer = std::move(cmd.commandText);

		// do something usefull with recived data

		ServerResponse resp{ 250, { "OK: message accepted" } };

		self->m_mail_from.clear();
		self->m_recipients.clear();
		self->m_data_buffer.clear();
		self->m_state = State::READY;
		self->send_response(resp);
		});
}

bool SMTPServerSession::extract_address(std::string& s)
{
	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.front())))
		s.erase(s.begin());

	while (!s.empty() && std::isspace(static_cast<unsigned char>(s.back())))
		s.pop_back();

	if (s.empty())
		return false;

	if (s.front() == '<')
		s.erase(s.begin());

	if (!s.empty() && s.back() == '>')
		s.pop_back();

	return !s.empty();
}
