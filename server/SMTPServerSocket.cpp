#include "SMTPServerSocket.h"
#include "SMTPCommandParser.h"

using boost::asio::ip::tcp;

SMTPServerSocket::SMTPServerSocket(tcp::socket socket)
	: m_socket(std::move(socket)),
	m_strand(m_socket.get_executor()),
	m_timer(m_socket.get_executor()),
	m_buf(),
	m_last_error(),
	m_closing(false)
{}

SMTPServerSocket::~SMTPServerSocket() {
	boost::system::error_code ignored;
	m_timer.cancel();
	if (m_socket.is_open()) {
		m_socket.shutdown(tcp::socket::shutdown_both, ignored);
		m_socket.close(ignored);
	}
}

std::error_code SMTPServerSocket::last_error() const {
	return m_last_error;
}

void SMTPServerSocket::close() {
	auto self = shared_from_this();
	boost::asio::post(m_strand, [self]() {
		if (self->m_closing) return;
		self->m_closing = true;
		boost::system::error_code ignored;
		self->m_timer.cancel();
		self->m_socket.cancel(ignored);
		if (self->m_socket.is_open()) {
			self->m_socket.shutdown(tcp::socket::shutdown_both, ignored);
			self->m_socket.close(ignored);
		}
		});
}

static inline void build_response_string(const ServerResponse& resp, std::string& out) {
	if (resp.lines.empty()) {
		out = std::to_string(resp.code) + "\r\n";
	}
	else if (resp.lines.size() == 1) {
		out = std::to_string(resp.code) + " " + resp.lines.front() + "\r\n";
	}
	else {
		for (size_t i = 0; i < resp.lines.size(); ++i) {
			if (i + 1 < resp.lines.size())
				out += std::to_string(resp.code) + "-" + resp.lines[i] + "\r\n";
			else
				out += std::to_string(resp.code) + " " + resp.lines[i] + "\r\n";
		}
	}
}

void SMTPServerSocket::sendResponse(const ServerResponse& resp, WriteHandler handler) {
	auto self = shared_from_this();
	std::string out;
	build_response_string(resp, out);

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

static inline std::string extract_from_streambuf(boost::asio::streambuf& buf, std::size_t n) {
	auto data = buf.data();
	std::string out(boost::asio::buffers_begin(data),
		boost::asio::buffers_begin(data) + static_cast<std::ptrdiff_t>(n));
	buf.consume(n);
	return out;
}

void SMTPServerSocket::readCommand(std::chrono::milliseconds timeout, ReadHandler handler) {
	auto self = shared_from_this();

	boost::asio::post(m_strand, [self, timeout, handler = std::move(handler)]() mutable {
		if (self->m_closing) {
			if (handler) {
				boost::asio::post(self->m_strand, [handler = std::move(handler)]() mutable {
					handler(ClientCommand{}, boost::asio::error::operation_aborted);
					});
			}
			return;
		}

		auto completed = std::make_shared<bool>(false);

		if (timeout.count() > 0) {
			self->m_timer.expires_after(timeout);
			self->m_timer.async_wait(boost::asio::bind_executor(self->m_strand, [self, completed](const boost::system::error_code& ec) {
				if (!ec && !*completed) {
					boost::system::error_code ignored;
					self->m_socket.cancel(ignored);
				}
				}));
		}

		boost::asio::async_read_until(self->m_socket, self->m_buf, "\r\n",
			boost::asio::bind_executor(self->m_strand,
				[self, completed, handler = std::move(handler)](const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {
					*completed = true;
					self->m_timer.cancel();

					if (ec) {
						self->m_last_error = ec;
						if (handler) {
							boost::asio::post(self->m_strand, [handler = std::move(handler), ec]() mutable {
								handler(ClientCommand{}, ec);
								});
						}
						return;
					}

					std::string line = extract_from_streambuf(self->m_buf, bytes_transferred);

					ClientCommand cmd;
					SMTPCommandParser::parseLine(line, cmd);

					if (handler) {
						boost::asio::post(self->m_strand, [handler = std::move(handler), cmd = std::move(cmd)]() mutable {
							handler(std::move(cmd), {});
							});
					}
				}));
		});
}


static inline ClientCommand dot_unstuff(std::string& buf) {

	const std::string term = "\r\n.\r\n";
	if (buf.size() >= term.size() && buf.compare(buf.size() - term.size(), term.size(), term) == 0) {
		buf.erase(buf.size() - term.size());
	}
	else {
		const std::string alt = ".\r\n";
		if (buf.size() >= alt.size() && buf.compare(buf.size() - alt.size(), alt.size(), alt) == 0) {
			buf.erase(buf.size() - alt.size());
		}
	}

	std::string out;
	out.reserve(buf.size());

	size_t pos = 0;
	while (pos < buf.size()) {
		size_t lf = buf.find('\n', pos);
		std::string_view line;
		if (lf == std::string::npos) {
			line = std::string_view(buf.data() + pos, buf.size() - pos);
			pos = buf.size();
		}
		else {
			line = std::string_view(buf.data() + pos, lf - pos);
			pos = lf + 1;
		}

		if (!line.empty() && line.back() == '\r') {
			line = std::string_view(line.data(), line.size() - 1);
		}

		if (line.size() >= 2 && line[0] == '.' && line[1] == '.') {
			out.append(line.data() + 1, line.size() - 1);
		}
		else {
			out.append(line.data(), line.size());
		}
		out += "\r\n";
	}

	ClientCommand cmd(CommandType::SEND_DATA, std::move(out));
	return cmd;
}


void SMTPServerSocket::readDataBlock(std::chrono::milliseconds timeout, ReadHandler handler) {
	auto self = shared_from_this();
	boost::asio::post(m_strand, [self, timeout, handler = std::move(handler)]() mutable {
		if (self->m_closing) {
			if (handler) {
				boost::asio::post(self->m_strand, [handler = std::move(handler)]() mutable {
					handler(ClientCommand{}, boost::asio::error::operation_aborted);
					});
			}
			return;
		}

		auto completed = std::make_shared<bool>(false);
		if (timeout.count() > 0) {
			self->m_timer.expires_after(timeout);
			self->m_timer.async_wait(boost::asio::bind_executor(self->m_strand, [self, completed](const boost::system::error_code& ec) {
				if (!ec && !*completed) {
					boost::system::error_code ignored;
					self->m_socket.cancel(ignored);
				}
				}));
		}

		boost::asio::async_read_until(self->m_socket, self->m_buf, "\r\n.\r\n",
			boost::asio::bind_executor(self->m_strand,
				[self, completed, handler = std::move(handler)](const boost::system::error_code& ec, std::size_t bytes_transferred) mutable {
					*completed = true;
					self->m_timer.cancel();

					if (ec) {
						self->m_last_error = ec;
						if (handler) {
							boost::asio::post(self->m_strand, [handler = std::move(handler), ec]() mutable {
								handler(ClientCommand{}, ec);
								});
						}
						return;
					}

					std::string line = extract_from_streambuf(self->m_buf, bytes_transferred);

					ClientCommand cmd = dot_unstuff(line);

					if (handler) {
						boost::asio::post(self->m_strand, [handler = std::move(handler), cmd = std::move(cmd)]() mutable {
							handler(std::move(cmd), {});
							});
					}
				}));
		});
}

