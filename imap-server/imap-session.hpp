#pragma once
#include <boost/asio.hpp>
#include <memory>

class IMAP_Session : public std::enable_shared_from_this<IMAP_Session>
{
public:
	IMAP_Session(boost::asio::ip::tcp::socket socket);
	void start();

private:
	void sendBanner();
	void readCommand();
	void handleCommand(const std::string& line);
	void writeResponse(const std::string& msg);

	boost::asio::ip::tcp::socket socket_;
	boost::asio::streambuf buffer_;

	enum class State
	{
		NON_AUTH,
		AUTH,
		SELECTED
	} state_ = State::NON_AUTH;
};