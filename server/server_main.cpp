#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "FileStrategy.h"
#include "Logger.h"
#include "DataBaseManager.h"
#include "ServerSecureChannel.hpp"
#include "SmtpSession.hpp"
#include "SocketAcceptor.hpp"
#include "SocketConnection.hpp"

int main()
{
	constexpr uint16_t PORT = 25000;
	const std::string SERVER_DOMAIN = "localhost";

	try
	{
		DataBaseManager db("mail.db", "../../database/scheme/001_init_scheme.sql");
		if (!db.isConnected())
		{
			std::cerr << "Database connection failed\n";
			return 1;
		}
		UserDAL user_dal(db.getDB());
		UserRepository user_repo(user_dal);
		MessageRepository message_repo(db);

		Logger logger(std::make_unique<FileStrategy>(LogLevel::TRACE));

		boost::asio::io_context io_context;

		SocketAcceptor acceptor;

		if (!acceptor.Initialize(io_context, PORT))
		{
			std::cerr << "Failed to initialize acceptor\n";
			return 1;
		}

		std::cout << "SMTP Server running on port " << PORT << std::endl;

		while (true)
		{
			std::unique_ptr<SocketConnection> connection;

			if (!acceptor.Accept(connection) || !connection) continue;

			ServerSecureChannel channel(*connection);
			channel.setLogger(&logger);

			SmtpSession session(SERVER_DOMAIN, &message_repo, &user_repo);

			if (!channel.Send(session.Greeting()))
			{
				connection->Close();
				continue;
			}

			std::string input;

			while (channel.Receive(input))
			{
				std::string response = session.ProcessLine(input);

				if (!response.empty())
				{
					if (!channel.Send(response)) break;
				}

				if (session.getState() == SmtpState::STARTTLS)
				{
					if (!channel.StartTLS())
					{
						std::cerr << "TLS handshake failed\n";
						break;
					}

					session.ResetToHelo();
				}

				if (session.IsClosed()) break;
			}

			connection->Close();
		}
	}
	catch (const std::exception& exception)
	{
		std::cerr << "Server error: " << exception.what() << std::endl;
	}

	return 0;
}
