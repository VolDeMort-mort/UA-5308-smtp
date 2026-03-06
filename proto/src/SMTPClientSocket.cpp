#include "SMTPClientSocket.h"

#include "SMTPResponseParser.h"

using boost::asio::ip::tcp;

SMTPClientSocket::SMTPClientSocket(boost::asio::io_context& ioc)
	: m_socket(std::make_unique<PlainStream>(boost::asio::ip::tcp::socket(ioc)))
	, m_strand(m_socket->get_executor())
	, m_timer(m_socket->get_executor())
	, m_buf()
	, m_closing(false)
{
}

SMTPClientSocket::~SMTPClientSocket()
{
	m_timer.cancel();
}

void SMTPClientSocket::Close()
{
	auto self = shared_from_this();
	boost::asio::post(
		m_strand,
		[self]()
		{
			if (self->m_closing) return;
			self->m_closing = true;
			self->m_timer.cancel();
			if (self->m_socket)
			{
				self->m_socket->shutdown();
			}			  
		});
}

void SMTPClientSocket::Connect(const tcp::endpoint& ep, std::function<void(const boost::system::error_code&)> handler)
{
	auto self = shared_from_this();
	tcp::socket socket = m_socket->release_tcp_socket();
	auto socket_ptr = std::make_shared<tcp::socket>(std::move(socket));

	socket_ptr->async_connect(
		ep, boost::asio::bind_executor(
				m_strand,
				[self, socket_ptr, handler = std::move(handler)](const boost::system::error_code& ec) mutable
				{
					self->m_socket = std::make_unique<PlainStream>(std::move(*socket_ptr));
					if (handler) handler(ec);
				}));
}

static inline void DotStuff(std::string& s)
{
	if (s.empty())
	{
		s = ".\r\n";
		return;
	}

	std::string out;
	out.reserve(s.size() + 64);

	size_t pos = 0;
	while (pos < s.size())
	{
		size_t lf = s.find('\n', pos);
		std::string_view line;
		if (lf == std::string::npos)
		{
			line = std::string_view(s.data() + pos, s.size() - pos);
			pos = s.size();
		}
		else
		{
			line = std::string_view(s.data() + pos, lf - pos);
			pos = lf + 1;
		}

		if (!line.empty() && line.back() == '\r')
		{
			line = std::string_view(line.data(), line.size() - 1);
		}

		if (!line.empty() && line.front() == '.') out.push_back('.');

		out.append(line.data(), line.size());
		out += "\r\n";
	}

	out += ".\r\n";

	s.swap(out);
}

void SMTPClientSocket::CreateCommandString(const ClientCommand& cmd, std::string& out)
{
	switch (cmd.command)
	{
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
		DotStuff(out);
		break;
	default:
		out = "UNKNOWN\r\n";
		break;
	}
}

void SMTPClientSocket::SendCommand(const ClientCommand& cmd, WriteHandler handler)
{
	auto self = shared_from_this();
	std::string out;
	CreateCommandString(cmd, out);

	auto buf = std::make_shared<std::string>(std::move(out));

	boost::asio::post(
		m_strand,
		[self, buf, handler = std::move(handler)]() mutable
		{
			if (self->m_closing)
			{
				if (handler) handler(boost::asio::error::operation_aborted);
				return;
			}

			self->m_socket->async_write(
				boost::asio::buffer(*buf),
				boost::asio::bind_executor(
					self->m_strand,
					[self, buf, handler = std::move(handler)](const boost::system::error_code& ec, std::size_t) mutable
					{
						if (handler) handler(ec);
					}));
		});
}

void SMTPClientSocket::ReadResponse(std::chrono::milliseconds timeout, ReadHandler handler)
{
	auto self = shared_from_this();

	boost::asio::post(
		m_strand,
		[self, timeout, handler = std::move(handler)]() mutable
		{
			if (self->m_closing)
			{
				if (handler) handler(ServerResponse{}, boost::asio::error::operation_aborted);
				return;
			}

			auto lines = std::make_shared<std::vector<std::string>>();
			auto expected_code = std::make_shared<int>(-1);
			auto completed = std::make_shared<bool>(false);

			if (timeout.count() > 0)
			{
				self->m_timer.expires_after(timeout);
				self->m_timer.async_wait(boost::asio::bind_executor(
					self->m_strand,
					[self, completed](const boost::system::error_code& ec)
					{
						if (!ec && !*completed) self->m_socket->cancel();
					}));
			}

			auto read_one = std::make_shared<std::function<void()>>();
			*read_one = [self, lines, expected_code, completed, handler, read_one, timeout]() mutable
			{
				self->m_socket->async_read_until(
					self->m_buf, "\r\n",
					boost::asio::bind_executor(
						self->m_strand,
						[self, lines, expected_code, completed, handler, read_one,
						 timeout](const boost::system::error_code& ec, std::size_t bytes_transferred) mutable
						{
							if (ec)
							{
								*completed = true;
								self->m_timer.cancel();
								if (handler) handler(ServerResponse{}, ec);
								return;
							}

							auto data = self->m_buf.data();

							std::string line(boost::asio::buffers_begin(data),
											 boost::asio::buffers_begin(data) + bytes_transferred);

							self->m_buf.consume(bytes_transferred);

							if (!line.empty() && line.back() == '\n') line.pop_back();
							if (!line.empty() && line.back() == '\r') line.pop_back();

							lines->push_back(line);

							if (*expected_code == -1)
							{
								int code = 0;
								if (SMTPResponseParser::TryParseCode(line, code))
								{
									*expected_code = code;
								}
								else
								{
									*completed = true;
									self->m_timer.cancel();
									boost::system::error_code ec = make_error_code(boost::system::errc::protocol_error);
									if (handler) handler(ServerResponse{}, ec);
									return;
								}
							}

							bool is_final = SMTPResponseParser::IsFinalSep(line);

							if (is_final)
							{
								ServerResponse resp;
								SMTPResponseParser::ParseLine(*lines, resp);
								*completed = true;
								self->m_timer.cancel();
								if (handler) handler(resp, {});
								return;
							}

							if (timeout.count() > 0)
							{
								self->m_timer.expires_after(timeout);
							}

							boost::asio::post(self->m_strand, [read_one]() mutable { (*read_one)(); });
						}));
			};

			(*read_one)();
		});
}

void SMTPClientSocket::StartTls(boost::asio::ssl::context& ctx,
								std::function<void(const boost::system::error_code&)> handler)
{
	auto self = shared_from_this();

	boost::asio::dispatch(
		m_strand,
		[self, &ctx, handler = std::move(handler)]() mutable
		{
			if (self->m_is_tls)
			{
				handler(make_error_code(boost::system::errc::already_connected));
				return;
			}

			if (self->m_buf.size() > 0)
			{
				self->m_buf.consume(self->m_buf.size());
			}

			tcp::socket raw_socket = self->m_socket->release_tcp_socket();
			self->m_socket.reset();

			auto ssl = std::make_unique<boost::asio::ssl::stream<tcp::socket>>(std::move(raw_socket), ctx);

			ssl->async_handshake(
				boost::asio::ssl::stream_base::client,
				boost::asio::bind_executor(
					self->m_strand,
					[self, ssl = std::move(ssl),
					 handler = std::move(handler)](const boost::system::error_code& ec) mutable
					{
						if (ec)
						{
							handler(ec);
							return;
						}

						self->m_socket = std::make_unique<SslStream>(std::move(*ssl));

						self->m_is_tls = true;

						handler(ec);
					}));
		});
}