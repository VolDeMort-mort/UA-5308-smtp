#include <boost/asio.hpp>
#include <gtest/gtest.h>

#include "SMTPServer.h"

using boost::asio::ip::tcp;

// using syncchronus opperations for easier syntax

void SendSmtpCommand(tcp::socket& sock, const std::string& cmd)
{
	boost::asio::write(sock, boost::asio::buffer(cmd));
}

int ReadSmtpResponse(tcp::socket& sock)
{
	boost::asio::streambuf buf;
	std::istream is(&buf);

	boost::asio::read_until(sock, buf, "\r\n");

	std::string line;
	std::getline(is, line);

	if (line.size() < 3) throw std::runtime_error("Invalid SMTP response");

	return std::stoi(line.substr(0, 3));
}

TEST(SMTP_Server_Tests, Correct_Command_Sequence)
{
	using boost::asio::ip::tcp;

	boost::asio::io_context ioc;

	SMTPServer server(ioc, 2526);
	server.run(1);

	tcp::socket sock(ioc);
	sock.connect({boost::asio::ip::address_v4::loopback(), 2526});

	EXPECT_EQ(ReadSmtpResponse(sock), 220);

	SendSmtpCommand(sock, "HELO client\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 250);

	SendSmtpCommand(sock, "MAIL FROM:<a@test.com>\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 250);

	SendSmtpCommand(sock, "RCPT TO:<b@test.com>\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 250);

	SendSmtpCommand(sock, "DATA\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 354);

	SendSmtpCommand(sock, "Hello world\r\n.\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 250);

	SendSmtpCommand(sock, "QUIT\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 221);

	server.Stop();
}

TEST(SMTP_Server_Tests, Incorrect_Command_Sequence)
{
	using boost::asio::ip::tcp;

	boost::asio::io_context ioc;

	SMTPServer server(ioc, 2527);
	server.run(1);

	tcp::socket sock(ioc);
	sock.connect({boost::asio::ip::address_v4::loopback(), 2527});

	EXPECT_EQ(ReadSmtpResponse(sock), 220);

	SendSmtpCommand(sock, "HELO client\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 250);

	SendSmtpCommand(sock, "RCPT TO:<b@test.com>\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 503);

	SendSmtpCommand(sock, "DATA\r\n");
	EXPECT_EQ(ReadSmtpResponse(sock), 503);

	server.Stop();
}