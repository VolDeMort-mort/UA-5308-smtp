#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <sodium.h>

#include "FileStrategy.h"
#include "Logger.h"
#include "DataBaseManager.h"
#include "schema.h"
#include "ThreadPool.h"
#include "ServerSecureChannel.hpp"
#include "SmtpSession.hpp"
#include "SocketAcceptor.hpp"
#include "SocketConnection.hpp"

int main()
{
	if (sodium_init() < 0) {
		std::cerr << "Failed to initialize libsodium\n";
		return 1;
	}

	constexpr uint16_t PORT = 25000;
	const std::string SERVER_DOMAIN = "localhost";

	try
	{
		DataBaseManager db("mail.db", initSchema());
		if (!db.isConnected())
		{
			std::cerr << "Database connection failed\n";
			return 1;
		}
		UserDAL user_dal(db.getDB(), db.pool());
		UserRepository user_repo(db);
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

		ThreadPool pool;

        constexpr int WORKER_THREADS = 4;

        pool.initialize(WORKER_THREADS);

		while (true)
        {
            std::unique_ptr<SocketConnection> connection;

            if (!acceptor.Accept(connection) || !connection)
                continue;

            std::shared_ptr<SocketConnection> shared_connection =
                std::shared_ptr<SocketConnection>(std::move(connection));

            pool.add_task([shared_connection,
                           SERVER_DOMAIN,
                           &message_repo,
                           &user_repo,
                           &logger]()
            {
                ServerSecureChannel channel(*shared_connection);
                channel.setLogger(&logger);

                SmtpSession session(SERVER_DOMAIN,
                                    &message_repo,
                                    &user_repo,
                                    &logger);

                if (!channel.Send(session.Greeting()))
                {
                    shared_connection->Close();
                    return;
                }

                std::string input;

                while (channel.Receive(input))
                {
                    std::string response =
                        session.ProcessLine(input);

                    if (!response.empty())
                    {
                        if (!channel.Send(response))
                            break;
                    }

                    if (session.getState() == SmtpState::STARTTLS)
                    {
                        if (!channel.StartTLS())
                        {
                            std::cerr << "TLS handshake failed\n";
                            break;
                        }
						session.SetSecure(true);
                        session.ResetToHelo();
                    }

                    if (session.IsClosed())
                        break;
                }

                shared_connection->Close();
            });
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Server error: "
                  << exception.what()
                  << std::endl;
    }

    return 0;
}
