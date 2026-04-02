#include "SocketConnection.hpp"

#ifdef _WIN32

#include <winsock2.h>
#include <ws2tcpip.h>

#else

#include <sys/select.h>
#include <unistd.h>

#endif

using boost::asio::ip::tcp;

SocketConnection::SocketConnection(tcp::socket socket) : m_socket(std::move(socket)) {}

void SocketConnection::SetTimeout(int seconds)
{
	if (seconds > 0) m_timeout_seconds = seconds;
}

bool SocketConnection::WaitForEvent(bool for_read, int t_seconds /* =-1 */)
{
	if (!m_socket.is_open()) return false;

	auto fd = m_socket.native_handle();

	fd_set read_set;
	fd_set write_set;

	FD_ZERO(&read_set);
	FD_ZERO(&write_set);

	if (for_read)
		FD_SET(fd, &read_set);
	else
		FD_SET(fd, &write_set);

	timeval timeout;

	if (t_seconds == -1)
	{
		timeout.tv_sec = m_timeout_seconds;
	}
	else
	{
		timeout.tv_sec = t_seconds;
	}

	timeout.tv_usec = 0;

#ifdef _WIN32
	int result = select(0, for_read ? &read_set : nullptr, for_read ? nullptr : &write_set, nullptr, &timeout);
#else
	int result = select(fd + 1, for_read ? &read_set : nullptr, for_read ? nullptr : &write_set, nullptr, &timeout);
#endif

	if (result <= 0)
	{
		Close();
		return false;
	}

	return true;
}

bool SocketConnection::Receive(std::string& out_data)
{
	if (!m_socket.is_open()) return false;

	if (m_buffer.size() == 0) 
	{
		if (!WaitForEvent(true)) return false;
	}

	boost::system::error_code error;

	boost::asio::read_until(m_socket, m_buffer, "\r\n", error);

	if (error)
	{
		Close();
		return false;
	}

	if (m_buffer.size() > MAX_LINE_SIZE)
	{
		Close();
		return false;
	}

	std::istream stream(&m_buffer);
	std::getline(stream, out_data);

	if (!out_data.empty() && out_data.back() == '\r') out_data.pop_back();

	return true;
}

bool SocketConnection::Send(const std::string& data)
{
	if (!m_socket.is_open()) return false;

	if (data.empty()) return false;

	if (!WaitForEvent(false)) return false;

	boost::system::error_code error;

	boost::asio::write(m_socket, boost::asio::buffer(data), error);

	if (error)
	{
		Close();
		return false;
	}

	return true;
}

bool SocketConnection::Close()
{
	if (!m_socket.is_open()) return false;

	boost::system::error_code error;

	m_socket.shutdown(tcp::socket::shutdown_both, error);
	m_socket.close(error);

	return true;
}

bool SocketConnection::IsOpen() const noexcept
{
	return m_socket.is_open();
}


bool SocketConnection::ReceiveRaw(unsigned char* buffer, std::size_t size)
{
	if (!m_socket.is_open())
	{
		return false;
	}

	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(m_timeout_seconds);

	std::size_t total = 0;

	while (total < size)
	{
		auto now = std::chrono::steady_clock::now();
		if (now >= deadline)
		{
			Close();
			return false;
		}

		auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
		if (!WaitForEvent(true, remaining))
		{
			return false;
		}

		boost::system::error_code error_code;

		std::size_t received = m_socket.read_some(boost::asio::buffer(buffer + total, size - total), error_code);

		if (error_code)
		{
			Close();
			return false;
		}

		total += received;
	}
	return true;
}
	

bool SocketConnection::SendRaw(const unsigned char* buffer, std::size_t size)
{
	if (!m_socket.is_open())
	{
		return false;
	}

	auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(m_timeout_seconds);

	std::size_t total = 0;

	while (total < size)
	{
		auto now = std::chrono::steady_clock::now();
		if (now >= deadline)
		{
			Close();
			return false;
		}

		auto remaining = std::chrono::duration_cast<std::chrono::seconds>(deadline - now).count();
		if (!WaitForEvent(false, remaining))
		{
			return false;
		}

		boost::system::error_code error_code;

		std::size_t sent = m_socket.write_some(boost::asio::buffer(buffer + total, size - total), error_code);

		if (error_code)
		{
			Close();
			return false;
		}

		total += sent;
	}
	return true;
}

