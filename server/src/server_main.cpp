#include <boost/asio.hpp>
#include <filesystem>
#include <iostream>

#include "SMTPServer.h"

int main()
{
	// variables that can be loaded with configuration loader
	const short port = 25000;

	const size_t MAX_SMTP_LINE_SERVER = MAX_SMTP_LINE;
	const size_t MAX_DATA_SIZE_SERVER = MAX_DATA_SIZE;

	const std::chrono::seconds CMD_TIMEOUT_SERVER = std::chrono::seconds(300);
	const std::chrono::seconds DATA_TIMEOUT_SERVER = std::chrono::seconds(600);


	try
	{
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

		SMTPServer server(ioc, port, cert_file.string(), key_file.string(), CMD_TIMEOUT_SERVER, DATA_TIMEOUT_SERVER,
						  MAX_SMTP_LINE_SERVER, MAX_DATA_SIZE_SERVER);
		server.run(std::thread::hardware_concurrency());

		std::cout << "SMTP server started on port 25000\n";
		std::cout << "Type 'stop' and press Enter to shutdown\n";

		std::string input;
		while (std::getline(std::cin, input))
		{
			if (input == "stop")
			{
				std::cout << "Stopping server...\n";
				server.Stop();
				break;
			}
			else
			{
				std::cout << "Unknown command. Type 'stop' to shutdown\n";
			}
		}
	}
	catch (const std::exception& ex)
	{
		std::cerr << "Fatal error: " << ex.what() << std::endl;
	}

	return 0;
}
