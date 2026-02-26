#include <boost/asio.hpp>
#include <iostream>

#include "SMTPServer.h"

int main() {
    try {
        boost::asio::io_context ioc;

        SMTPServer server(ioc, 25000);
        server.run(std::thread::hardware_concurrency());

        std::cout << "SMTP server started on port 25000\n";
        std::cout << "Type 'stop' and press Enter to shutdown\n";

        std::string input;
        while (std::getline(std::cin, input)) {
            if (input == "stop") {
                std::cout << "Stopping server...\n";
                server.soft_stop();
                break;
            }
            else {
                std::cout << "Unknown command. Type 'stop' to shutdown\n";
            }
        }
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
    }

    return 0;
}

