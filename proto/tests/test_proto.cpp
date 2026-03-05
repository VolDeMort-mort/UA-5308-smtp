#include <boost/asio.hpp>
#include <chrono>
#include <functional>
#include <future>
#include <gtest/gtest.h>
#include <thread>
#include <vector>

#include "../include/SMTPClientSocket.h"
#include "../include/SMTPServerSocket.h"
#include "../include/SMTP_Types.h"

//asynchronus operations are synchronized in tests with future/promise

using namespace std::chrono_literals;
using boost::asio::ip::tcp;

template <typename T> static T WaitFuture(std::future<T>& f, std::chrono::milliseconds timeout = 2000ms)
{
	if (f.wait_for(timeout) != std::future_status::ready)
	{
		throw std::runtime_error("future timeout");
	}

	return f.get();
}

struct SmtpTestSockets
{
	boost::asio::io_context ioc;
	boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work;
	std::thread io_thread;

	tcp::acceptor acceptor;
	std::shared_ptr<SMTPServerSocket> server_socket;
	std::shared_ptr<SMTPClientSocket> client_socket;

	SmtpTestSockets() : work(boost::asio::make_work_guard(ioc)), acceptor(ioc, tcp::endpoint(tcp::v4(), 2525))
	{
		io_thread = std::thread([this]() { ioc.run(); });

		client_socket = std::make_shared<SMTPClientSocket>(ioc);

		auto server_ready_p = std::make_shared<std::promise<void>>();
		auto server_ready_f = server_ready_p->get_future();

		acceptor.async_accept(
			[this, server_ready_p](const boost::system::error_code& ec, tcp::socket sock)
			{
				if (ec)
				{
					server_ready_p->set_exception(
						std::make_exception_ptr(std::runtime_error("accept failed: " + ec.message())));
					return;
				}
				server_socket = std::make_shared<SMTPServerSocket>(std::move(sock));
				server_ready_p->set_value();
			});

		auto endpoint = tcp::endpoint(boost::asio::ip::address_v4::loopback(), acceptor.local_endpoint().port());
		auto client_connected_p = std::make_shared<std::promise<void>>();
		auto client_connected_f = client_connected_p->get_future();

		client_socket->Connect(endpoint,
							  [client_connected_p](const boost::system::error_code& ec)
							  {
								  if (ec)
									  client_connected_p->set_exception(std::make_exception_ptr(
										  std::runtime_error("connect failed: " + ec.message())));
								  else
									  client_connected_p->set_value();
							  });

		server_ready_f.get();
		client_connected_f.get();
	}

	void start_tls_on_both(boost::asio::ssl::context& server_ctx, boost::asio::ssl::context& client_ctx)
	{
		auto server_p = std::make_shared<std::promise<void>>();
		auto client_p = std::make_shared<std::promise<void>>();

		auto server_f = server_p->get_future();
		auto client_f = client_p->get_future();

		server_socket->StartTls(server_ctx,
								[server_p](const boost::system::error_code& ec)
								{
									if (ec)
										server_p->set_exception(std::make_exception_ptr(
											std::runtime_error("server tls failed: " + ec.message())));
									else
										server_p->set_value();
								});

		client_socket->StartTls(client_ctx,
								[client_p](const boost::system::error_code& ec)
								{
									if (ec)
										client_p->set_exception(std::make_exception_ptr(
											std::runtime_error("client tls failed: " + ec.message())));
									else
										client_p->set_value();
								});

		WaitFuture(server_f);
		WaitFuture(client_f);
	}

	~SmtpTestSockets()
	{
		work.reset();
		ioc.stop();
		if (io_thread.joinable()) io_thread.join();
	}
};

TEST(SMTP_Socket_Tests, ClientToServer_AllCommandsParsed)
{
	SmtpTestSockets sockets;

	std::vector<ClientCommand> commands = {{CommandType::HELO, "HELO text"},
										   {CommandType::EHLO, "EHLO text"},
										   {CommandType::MAIL, "sender@example.com"},
										   {CommandType::RCPT, "recipient@example.com"},
										   {CommandType::DATA, ""},
										   {CommandType::SEND_DATA, "Hello\r\n"},
										   {CommandType::QUIT, ""}};

	for (const auto& cmd : commands)
	{
		auto parsed_p = std::make_shared<std::promise<ClientCommand>>();
		auto parsed_f = parsed_p->get_future();
		if (cmd.command == SEND_DATA)
		{
			sockets.server_socket->ReadDataBlock(
				10000ms,
				[parsed_p](ClientCommand parsed, const boost::system::error_code& ec)
				{
					if (ec)
						parsed_p->set_exception(
							std::make_exception_ptr(std::runtime_error("server ReadCommand ec: " + ec.message())));
					else
						parsed_p->set_value(std::move(parsed));
				});
		}
		else
		{
			sockets.server_socket->ReadCommand(
				10000ms,
				[parsed_p](ClientCommand parsed, const boost::system::error_code& ec)
				{
					if (ec)
						parsed_p->set_exception(
							std::make_exception_ptr(std::runtime_error("server ReadCommand ec: " + ec.message())));
					else
						parsed_p->set_value(std::move(parsed));
				});
		}
		auto write_p = std::make_shared<std::promise<void>>();
		auto write_f = write_p->get_future();

		sockets.client_socket->SendCommand(
			cmd,
			[write_p](const boost::system::error_code& ec)
			{
				if (ec)
					write_p->set_exception(std::make_exception_ptr(
					std::runtime_error("client SendCommand ec: " + ec.message())));
				else
					write_p->set_value();
			});							  							  

		WaitFuture(write_f);
		ClientCommand parsed = WaitFuture(parsed_f);

		EXPECT_EQ(parsed.command, cmd.command);
		EXPECT_EQ(parsed.commandText, cmd.commandText);
	}
}

TEST(SMTP_Socket_Tests, ServerToClient_AllResponsesParsed)
{
	SmtpTestSockets sockets;

	std::vector<ServerResponse> responses = {{220, {"smtp.example.com Service ready"}},
											 {250, {"OK"}},
											 {354, {"Start mail input; end with <CRLF>.<CRLF>"}},
											 {250, {"Queued as 12345", "Additional info"}}};

	for (const auto& resp : responses)
	{
		auto parsed_p = std::make_shared<std::promise<ServerResponse>>();
		auto parsed_f = parsed_p->get_future();

		sockets.client_socket->ReadResponse(2000ms,
										   [parsed_p](ServerResponse resp, const boost::system::error_code& ec)
										   {
											   if (ec)
												   parsed_p->set_exception(std::make_exception_ptr(
													   std::runtime_error("client ReadResponse ec: " + ec.message())));
											   else
												   parsed_p->set_value(std::move(resp));
										   });
										   

		auto write_p = std::make_shared<std::promise<void>>();
		auto write_f = write_p->get_future();

		sockets.server_socket->SendResponse(resp,
										   [write_p](const boost::system::error_code& ec)
										   {
											   if (ec)
												   write_p->set_exception(std::make_exception_ptr(
													   std::runtime_error("server SendResponse ec: " + ec.message())));
											   else
												   write_p->set_value();
										   });

		WaitFuture(write_f);
		ServerResponse parsed = WaitFuture(parsed_f);

		EXPECT_EQ(parsed.code, resp.code);
		EXPECT_EQ(parsed.lines.size(), resp.lines.size());
		for (size_t i = 0; i < resp.lines.size(); ++i)
		{
			EXPECT_EQ(parsed.lines[i], resp.lines[i]);
		}
	}
}

TEST(SMTP_Socket_Tests, TLS_Handshake)
{
	SmtpTestSockets sockets;

	boost::asio::ssl::context server_ctx(boost::asio::ssl::context::tls_server);
	server_ctx.use_certificate_chain_file("certs/server.crt");
	server_ctx.use_private_key_file("certs/server.key", boost::asio::ssl::context::pem);

	boost::asio::ssl::context client_ctx(boost::asio::ssl::context::tls_client);
	client_ctx.set_verify_mode(boost::asio::ssl::verify_none);

	sockets.start_tls_on_both(server_ctx, client_ctx);

	ClientCommand cmd{CommandType::HELO, "tls-test"};

	auto parsed_p = std::make_shared<std::promise<ClientCommand>>();
	auto parsed_f = parsed_p->get_future();

	sockets.server_socket->ReadCommand(2000ms,
									  [parsed_p](ClientCommand parsed, const boost::system::error_code& ec)
									  {
										  if (ec)
											  parsed_p->set_exception(
												  std::make_exception_ptr(std::runtime_error(ec.message())));
										  else
											  parsed_p->set_value(parsed);
									  });

	auto write_p = std::make_shared<std::promise<void>>();
	auto write_f = write_p->get_future();

	sockets.client_socket->SendCommand(cmd,
									  [write_p](const boost::system::error_code& ec)
									  {
										  if (ec)
											  write_p->set_exception(
												  std::make_exception_ptr(std::runtime_error(ec.message())));
										  else
											  write_p->set_value();
									  });

	WaitFuture(write_f);
	auto parsed = WaitFuture(parsed_f);

	EXPECT_EQ(parsed.command, cmd.command);
	EXPECT_EQ(parsed.commandText, cmd.commandText);
}