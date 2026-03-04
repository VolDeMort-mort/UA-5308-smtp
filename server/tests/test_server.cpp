#include <gtest/gtest.h>
#include <boost/asio.hpp>

#include <../include/SMTPServer.h>

using boost::asio::ip::tcp;

void send_smtp_command(tcp::socket& sock, const std::string& cmd) {
	boost::asio::write(sock, boost::asio::buffer(cmd));
}

int read_smtp_response(tcp::socket& sock) {
	boost::asio::streambuf buf;
	std::istream is(&buf);

	boost::asio::read_until(sock, buf, "\r\n");

	std::string line;
	std::getline(is, line);

	if (line.size() < 3) throw std::runtime_error("Invalid SMTP response");

	return std::stoi(line.substr(0, 3));
}

TEST(SMTP_Server_Tests, Correct_Command_Sequence){
	using boost::asio::ip::tcp;

	boost::asio::io_context ioc;

	SMTPServer server(ioc, 2526);
	server.run(1);

	tcp::socket sock(ioc);
	sock.connect({boost::asio::ip::address_v4::loopback(), 2526});

	EXPECT_EQ(read_smtp_response(sock), 220);

	send_smtp_command(sock, "HELO client\r\n");
	EXPECT_EQ(read_smtp_response(sock), 250);

	send_smtp_command(sock, "MAIL FROM:<a@test.com>\r\n");
	EXPECT_EQ(read_smtp_response(sock), 250);

	send_smtp_command(sock, "RCPT TO:<b@test.com>\r\n");
	EXPECT_EQ(read_smtp_response(sock), 250);

	send_smtp_command(sock, "DATA\r\n");
	EXPECT_EQ(read_smtp_response(sock), 354);

	send_smtp_command(sock, "Hello world\r\n.\r\n");
	EXPECT_EQ(read_smtp_response(sock), 250);

	send_smtp_command(sock, "QUIT\r\n");
	EXPECT_EQ(read_smtp_response(sock), 221);

	server.soft_stop();
}

TEST(SMTP_Server_Tests, Incorrect_Command_Sequence)
{
	using boost::asio::ip::tcp;

	boost::asio::io_context ioc;

	SMTPServer server(ioc, 2527);
	server.run(1);

	tcp::socket sock(ioc);
	sock.connect({boost::asio::ip::address_v4::loopback(), 2527});

	EXPECT_EQ(read_smtp_response(sock), 220);

	send_smtp_command(sock, "HELO client\r\n");
	EXPECT_EQ(read_smtp_response(sock), 250);

	send_smtp_command(sock, "RCPT TO:<b@test.com>\r\n");
	EXPECT_EQ(read_smtp_response(sock), 503);

	send_smtp_command(sock, "DATA\r\n");
	EXPECT_EQ(read_smtp_response(sock), 503);

	server.soft_stop();
}