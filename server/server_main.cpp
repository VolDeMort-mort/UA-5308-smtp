#include <iostream>
#include <memory>

#include <boost/asio.hpp>

#include "SocketAcceptor.hpp"
#include "SocketConnection.hpp"
#include "SmtpSession.hpp"

int main()
{
    constexpr uint16_t PORT = 25000;
    const std::string SERVER_DOMAIN = "localhost";

    try
    {
        boost::asio::io_context io_context;

        SocketAcceptor acceptor;

        if (!acceptor.Initialize(io_context, PORT))
        {
            std::cerr << "Failed to initialize acceptor\n";
            return 1;
        }

        std::cout << "SMTP Server running on port "
                  << PORT << std::endl;

        while (true)
        {
            std::unique_ptr<SocketConnection> connection;

            if (!acceptor.Accept(connection) || !connection)
                continue;

            SmtpSession session(SERVER_DOMAIN);

            if (!connection->Send(session.Greeting()))
            {
                connection->Close();
                continue;
            }

            std::string input;

            while (connection->Receive(input))
            {
                std::string response =
                    session.ProcessLine(input);

                if (!response.empty())
                {
                    if (!connection->Send(response))
                        break;
                }

                if (session.IsClosed())
                    break;
            }

            connection->Close();
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << "Server error: "
                  << exception.what() << std::endl;
    }

    return 0;
}
