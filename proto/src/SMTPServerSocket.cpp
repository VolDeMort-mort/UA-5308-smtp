#include "SMTPServerSocket.h"
#include "SMTPCommandParser.h"

using boost::asio::ip::tcp;

SMTPServerSocket::SMTPServerSocket(tcp::socket socket, std::size_t max_smtp_line, std::size_t max_data_size)
	: m_socket(std::make_unique<PlainStream>(std::move(socket)))
	, m_strand(m_socket->get_executor())
	, m_timer(m_socket->get_executor())
	, m_cmd_buf()
	, m_data_buf()
	, M_MAX_SMTP_LINE(max_smtp_line)
	, M_MAX_DATA_SIZE(max_data_size)
	, m_closing(false)
{
}

SMTPServerSocket::~SMTPServerSocket()
{
	m_timer.cancel();
}

void SMTPServerSocket::Close()
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

static inline void BuildResponseString(const ServerResponse& resp, std::string& out)
{
	if (resp.lines.empty())
	{
		out = std::to_string(resp.code) + "\r\n";
	}
	else if (resp.lines.size() == 1)
	{
		out = std::to_string(resp.code) + " " + resp.lines.front() + "\r\n";
	}
	else
	{
		for (size_t i = 0; i < resp.lines.size(); ++i)
		{
			if (i + 1 < resp.lines.size())
				out += std::to_string(resp.code) + "-" + resp.lines[i] + "\r\n";
			else
				out += std::to_string(resp.code) + " " + resp.lines[i] + "\r\n";
		}
	}
}

void SMTPServerSocket::SendResponse(const ServerResponse& resp, WriteHandler handler)
{
	auto self = shared_from_this();
	std::string out;
	BuildResponseString(resp, out);

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

void SMTPServerSocket::ReadCommand(std::chrono::milliseconds timeout, ReadHandler handler)
{
	auto self = shared_from_this();

	boost::asio::post(
		m_strand,
		[self, timeout, handler = std::move(handler)]() mutable
		{
			if (self->m_closing)
			{
				if (handler) handler(ClientCommand{}, boost::asio::error::operation_aborted);
				return;
			}

			auto completed = std::make_shared<bool>(false);

			if (timeout.count() > 0)
			{
				self->m_timer.expires_after(timeout);
				self->m_timer.async_wait(boost::asio::bind_executor(
					self->m_strand,
					[self, completed](const boost::system::error_code& ec)
					{
						if (!ec && !*completed)
						{
							self->m_socket->cancel();
						}
					}));
			}

			// now m_buf has max size, so, if the amount of the transfered data more then max_smtp_line
			// the read will end with "boost::asio::error::no_buffer_space"
			self->m_socket->async_read_until(
				self->m_cmd_buf, 
				self->M_MAX_SMTP_LINE, 
				"\r\n",
				boost::asio::bind_executor(
					self->m_strand,
					[self, completed, handler = std::move(handler)](const boost::system::error_code& ec,
																	std::size_t bytes_transferred) mutable
					{
						*completed = true;
						self->m_timer.cancel();

						if (ec)
						{
							if (handler) handler(ClientCommand{}, ec);
							return;
						}

						std::string_view line(self->m_cmd_buf.data(), bytes_transferred);

						ClientCommand cmd = SMTPCommandParser::ParseLine(line);

						self->m_cmd_buf.erase(0, bytes_transferred);

						if (handler) handler(std::move(cmd), {});
					}));
		});
}

void SMTPServerSocket::ReadDataBlock(std::chrono::milliseconds timeout, ReadHandler handler)
{
	auto self = shared_from_this();
	boost::asio::post(
		m_strand,
		[self, timeout, handler = std::move(handler)]() mutable
		{
			if (self->m_closing)
			{
				if (handler) handler(ClientCommand{}, boost::asio::error::operation_aborted);
				return;
			}

			auto completed = std::make_shared<bool>(false);
			if (timeout.count() > 0)
			{
				self->m_timer.expires_after(timeout);
				self->m_timer.async_wait(boost::asio::bind_executor(
					self->m_strand,
					[self, completed](const boost::system::error_code& ec)
					{
						if (!ec && !*completed)
						{
							self->m_socket->cancel();
						}
					}));
			}

			// temporary solution, becouse no db
			// now it allocate the hole messege in the memory
			// should read like 8-16 kb, write it to the db (or file and maybe then to db, maybe more efficient)
			self->m_socket->async_read_until(
				self->m_data_buf,
				self->M_MAX_DATA_SIZE,
				"\r\n.\r\n",
				boost::asio::bind_executor(
					self->m_strand,
					[self, completed, handler = std::move(handler)](const boost::system::error_code& ec,
																	std::size_t bytes_transferred) mutable
					{
						*completed = true;
						self->m_timer.cancel();

						if (ec)
						{
							if (handler) handler(ClientCommand{}, ec);
							return;
						}
						
						std::string_view line(self->m_data_buf.data(), bytes_transferred);

						ClientCommand cmd = SMTPCommandParser::ParseData(line);

						self->m_data_buf.erase(0, bytes_transferred);

						if (handler) handler(std::move(cmd), {});
					}));
		});
}

void SMTPServerSocket::StartTls(boost::asio::ssl::context& ctx,
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

			if (self->m_cmd_buf.size() > 0)
			{
				self->m_cmd_buf.clear();
			}

			if (self->m_data_buf.size() > 0)
			{
				self->m_data_buf.clear();
			}

			tcp::socket raw_socket = self->m_socket->release_tcp_socket();
			self->m_socket.reset();

			auto ssl = std::make_unique<boost::asio::ssl::stream<tcp::socket>>(std::move(raw_socket), ctx);

			ssl->async_handshake(
				boost::asio::ssl::stream_base::server,
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