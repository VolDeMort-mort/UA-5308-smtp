#include <boost/asio.hpp>
#include <iostream>
#include <filesystem>

#include "SMTPServer.h"

int main() {
    try {
        boost::asio::io_context ioc;

        std::filesystem::path base_dir;

        #ifdef _WIN32
		wchar_t buffer[MAX_PATH];
		GetModuleFileNameW(nullptr, buffer, MAX_PATH);
		base_dir = std::filesystem::path(buffer).parent_path();
        #else
		char result[PATH_MAX];
		ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
		base_dir = std::filesystem::path(std::string(result, count)).parent_path();
        #endif

		std::filesystem::path cert_file = base_dir / "certs" / "server.crt";
		std::filesystem::path key_file = base_dir / "certs" / "server.key";

        SMTPServer server(ioc, 25000, cert_file.string(), key_file.string());
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

