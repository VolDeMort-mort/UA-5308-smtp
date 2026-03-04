#include <boost/asio.hpp>
#include <iostream>
#include <memory>

#include "SmtpSession.hpp"
#include "SocketAcceptor.hpp"
#include "SocketConnection.hpp"
#include "ServerSecureChannel.hpp"
#include "ClientSecureChannel.hpp"
#include "SocketConnector.hpp"
#include "Logger.h"
#include "FileStrategy.h"

int main()
{
	try
	{
		Logger logger(std::make_unique<FileStrategy>(LogLevel::TRACE));
		constexpr uint16_t port = 25000;
		const std::string domain = "localhost";

		boost::asio::io_context ioContext;

		SocketAcceptor acceptor;

		if (!acceptor.initialize(ioContext, port))
		{
			std::cerr << "Failed to initialize acceptor\n";
			return 1;
		}
		
		std::cout << "SMTP Server running on port " << port << std::endl;

		while (true)
		{
			std::unique_ptr<SocketConnection> connection;

			if (!acceptor.accept(connection)) continue;

			ServerSecureChannel channel(*connection);
			channel.setLogger(&logger);

			SmtpSession session(domain);

			if (!channel.Send(session.greeting()))
			{
				connection->close();
				continue;
			}

			std::string input;

			while (channel.Receive(input))
			{
				std::string response = session.processLine(input);

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
					session.resetToHelo();
				}
				if (session.isClosed()) break;
			}
			connection->close();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Server error: " << e.what() << std::endl;
	}

	return 0;
}
