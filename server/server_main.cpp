#include <iostream>
#include <memory>
#include <boost/asio.hpp>

#include "SocketAcceptor.hpp"
#include "SocketConnection.hpp"
#include "SmtpSession.hpp"

int main()
{
    try
    {
        constexpr uint16_t PORT = 25000;
        const std::string DOMAIN = "localhost";

        boost::asio::io_context ioContext;

        SocketAcceptor acceptor;

        if (!acceptor.initialize(ioContext, PORT))
        {
            std::cerr << "Failed to initialize acceptor\n";
            return 1;
        }

        std::cout << "SMTP Server running on port "
                  << PORT << std::endl;

        while (true)
        {
            std::unique_ptr<SocketConnection> connection;

            if (!acceptor.accept(connection))
                continue;

            SmtpSession session(DOMAIN);

            if (!connection->send(session.greeting()))
            {
                connection->close();
                continue;
            }

            std::string input;

            while (connection->receive(input))
            {
                std::string response =
                    session.processLine(input);

                if (!response.empty())
                {
                    if (!connection->send(response))
                        break;
                }

                if (session.isClosed())
                    break;
            }

            connection->close();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Server error: "
                  << e.what() << std::endl;
    }

    return 0;
}

