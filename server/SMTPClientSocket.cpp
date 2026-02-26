#include "SMTPClientSocket.h"
#include "SMTPResponseParser.h"

using boost::asio::ip::tcp;

SMTPClientSocket::SMTPClientSocket(boost::asio::io_context& ioc)
	: m_socket(std::make_unique<PlainStream>(std::move(socket))), 
	m_strand(m_socket->get_executor()),
	m_timer(m_socket->get_executor()),
	m_buf(), 
	m_last_error(), 
	m_closing(false)
{}

SMTPClientSocket::~SMTPClientSocket() {
	boost::system::error_code ignored;
	m_timer.cancel();
	if (m_socket)
	{
		m_socket->shutdown();
	}
}

void SMTPClientSocket::close() {
	auto self = shared_from_this();
	boost::asio::post(m_strand,[self](){
		if (self->m_closing) return;
		self->m_closing = true;
		self->m_timer.cancel();
		self->m_socket->cancel();
		self->m_socket->shutdown();
		});
}

std::error_code SMTPClientSocket::last_error() const {
	return m_last_error;
}

void SMTPClientSocket::connect(const tcp::endpoint& ep, std::function<void(const boost::system::error_code&)> handler) {
	auto self = shared_from_this();
	tcp::socket socket = m_socket->release_tcp_socket();

	socket.async_connect(ep, boost::asio::bind_executor(m_strand,
		[self, handler = std::move(handler)](const boost::system::error_code& ec) mutable {
			if (ec) self->m_last_error = ec;
			if (handler) handler(ec);
		}));
}



static inline void dot_stuff(std::string& s) {
	if (s.empty()) {
		s = ".\r\n";
		return;
	}

	std::string out;
	out.reserve(s.size() + 64);

	size_t pos = 0;
	while (pos < s.size()) {
		size_t lf = s.find('\n', pos);
		std::string_view line;
		if (lf == std::string::npos) {
			line = std::string_view(s.data() + pos, s.size() - pos);
			pos = s.size();
		}
		else {
			line = std::string_view(s.data() + pos, lf - pos);
			pos = lf + 1;
		}

		if (!line.empty() && line.back() == '\r') {
			line = std::string_view(line.data(), line.size() - 1);
		}

		if (!line.empty() && line.front() == '.') out.push_back('.');

		out.append(line.data(), line.size());
		out += "\r\n";
	}

	out += ".\r\n";

	s.swap(out);
}


void SMTPClientSocket::createCommandString(const ClientCommand& cmd, std::string& out) {
	switch (cmd.command) {
	case CommandType::HELO:
		out = "HELO " + cmd.commandText + "\r\n";
		break;
	case CommandType::EHLO:
		out = "EHLO " + cmd.commandText + "\r\n";
		break;
	case CommandType::MAIL:
		out = "MAIL FROM:<" + cmd.commandText + ">\r\n";
		break;
	case CommandType::RCPT:
		out = "RCPT TO:<" + cmd.commandText + ">\r\n";
		break;
	case CommandType::DATA:
		out = "DATA\r\n";
		break;
	case CommandType::QUIT:
		out = "QUIT\r\n";
		break;
	case CommandType::SEND_DATA:
		out = cmd.commandText;
		dot_stuff(out);
		break;
	default:
		out = "UNKNOWN\r\n";
		break;
	}
}


void SMTPClientSocket::sendCommand(const ClientCommand& cmd, WriteHandler handler) {
	auto self = shared_from_this();
	std::string out;
	createCommandString(cmd, out);

	auto buf = std::make_shared<std::string>(std::move(out));

	boost::asio::post(m_strand, [self, buf, handler = std::move(handler)]() mutable {
		if (self->m_closing) {
			if (handler) {
				boost::asio::post(self->m_strand, [handler = std::move(handler)]() mutable {
					handler(boost::asio::error::operation_aborted);
					});
			}
			return;
		}

		boost::asio::async_write(self->m_socket, boost::asio::buffer(*buf),
			boost::asio::bind_executor(self->m_strand,
				[self, buf, handler = std::move(handler)](const boost::system::error_code& ec, std::size_t) mutable {
					if (ec) self->m_last_error = ec;
					if (handler) {
						boost::asio::post(self->m_strand, [handler = std::move(handler), ec]() mutable {
							handler(ec);
							});
					}
				}));
		});
}


void SMTPClientSocket::readResponse(std::chrono::milliseconds timeout, ReadHandler handler) {
	auto self = shared_from_this();

	boost::asio::post(m_strand, [self, timeout, handler = std::move(handler)]() mutable {
		if (self->m_closing) {
			if (handler) {
				boost::asio::post(self->m_strand, [handler = std::move(handler)]() mutable {
					handler(ServerResponse{}, boost::asio::error::operation_aborted);
					});
			}
			return;
		}

		auto lines = std::make_shared<std::vector<std::string>>();
		auto expected_code = std::make_shared<int>(-1);
		auto completed = std::make_shared<bool>(false);

		if (timeout.count() > 0) {
			self->m_timer.expires_after(timeout);
			self->m_timer.async_wait(boost::asio::bind_executor(self->m_strand,
				[self, completed](const boost::system::error_code& ec) {
					if (!ec && !*completed) {
						self->m_socket->cancel();
					}
				}));
		}

		auto read_one = std::make_shared<std::function<void()>>();
		*read_one = [self, lines, expected_code, completed, handler, read_one, timeout]() mutable {
			boost::asio::async_read_until(self->m_socket, self->m_buf, "\r\n",
				boost::asio::bind_executor(self->m_strand,
					[self, lines, expected_code, completed, handler, read_one, timeout]
					(const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {

						if (ec) {
							*completed = true;
							self->m_timer.cancel();
							self->m_last_error = ec;
							if (handler) {
								boost::asio::post(self->m_strand, [handler = std::move(handler), ec]() mutable {
									handler(ServerResponse{}, ec);
									});
							}
							return;
						}

						auto data = self->m_buf.data();

						std::string line(
							boost::asio::buffers_begin(data),
							boost::asio::buffers_begin(data) + bytes_transferred
						);

						self->m_buf.consume(bytes_transferred);

						if (!line.empty() && line.back() == '\n') line.pop_back();
						if (!line.empty() && line.back() == '\r') line.pop_back();

						lines->push_back(line);

						if (*expected_code == -1) {
							int code = 0;
							if (SMTPResponseParser::try_parse_code(line, code)) {
								*expected_code = code;
							}
							else {
								*completed = true;
								self->m_timer.cancel();
								boost::system::error_code perr = make_error_code(boost::system::errc::protocol_error);
								self->m_last_error = perr;
								if (handler) {
									boost::asio::post(self->m_strand, [handler = std::move(handler), perr]() mutable {
										handler(ServerResponse{}, perr);
										});
								}
								return;
							}
						}

						bool is_final = SMTPResponseParser::is_final_sep(line);

						if (is_final) {
							ServerResponse resp;
							SMTPResponseParser::parseLine(*lines, resp);
							*completed = true;
							self->m_timer.cancel();
							if (handler) {
								boost::asio::post(self->m_strand, [handler = std::move(handler), resp]() mutable {
									handler(resp, {});
									});
							}
							return;
						}

						if (timeout.count() > 0) {
							self->m_timer.expires_after(timeout);
						}

						boost::asio::post(self->m_strand, [read_one]() mutable {
							(*read_one)();
							});
					}));
			};

		(*read_one)();
		});
}

void SMTPClientSocket::start_tls(boost::asio::ssl::context& ctx, std::function<void(const boost::system::error_code&)> handler){
	if (m_is_tls) {
		handler(std::error_code);
		return;
	}
	tcp::socket raw_socket = m_socket->release_tcp_socket{};
	
	auto ssl = std::make_unique<boost::asio::ssl::stream<tcp::socket>>(std::move(raw_socket), ctx);
	auto self = shared_from_this();
	ssl->async_handshake(boost::asio::ssl::stream_base::client, 
		boost::asio::bind_executor(m_strand, [self, ssl = std::move(ssl), handler = std::move(handler)](const boost::system::error_code& ec) mutable{
			if(ec){
				handler(ec);
				return;
			}
			self->m_socket = std::make_unique<SslStream>(std::move(ssl));
			self->m_is_tls = true;
			handler(ec);
		}));
}