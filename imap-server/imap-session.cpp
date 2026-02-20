#include "imap-session.hpp"

IMAP_Session::IMAP_Session(boost::asio::ip::tcp::socket socket) : socket_(std::move(socket)) {}

void IMAP_Session::start()
{
	sendBanner();
}

void IMAP_Session::sendBanner()
{
	writeResponse("* OK IMAP Server Ready\r\n");
	readCommand();
}

void IMAP_Session::readCommand()
{
	auto self = shared_from_this();
	boost::asio::async_read_until(socket_, buffer_, "\r\n",
								  [this, self](std::error_code ec, std::size_t)
								  {
									  if (!ec)
									  {
										  std::istream is(&buffer_);
										  std::string line;
										  std::getline(is, line);
										  if (!line.empty() && line.back() == '\r') line.pop_back();
										  handleCommand(line);
									  }
								  });
}

void IMAP_Session::handleCommand(const std::string& line) {}

void IMAP_Session::writeResponse(const std::string& msg)
{
	auto self = shared_from_this();
	boost::asio::async_write(socket_, boost::asio::buffer(msg), [this, self](std::error_code, std::size_t) {});
}