#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <windows.h> // Тільки для Windows, щоб бачити кирилицю

#include "../common/mime/MimeParser.h"
#include "../common/models/Email.h"
#include "MockLogger.h"

int main()
{
	// Вмикаємо UTF-8 у консолі. БЕЗ ЦЬОГО ТИ БАЧИШ "КРАКОЗЯБРИ"!
	SetConsoleOutputCP(65001);

	std::cout << "=== ULTIMATE EML PARSER TEST ===\n\n";

	MockLogger systemLogger;

	// Вкажи шлях до свого файлу
	std::string filePath = "C:/Users/WellDone/Downloads/emlFile.eml";
	std::ifstream emlFile(filePath);

	if (!emlFile.is_open())
	{
		std::cerr << "Error: Could not open file: " << filePath << "\n";
		return 1;
	}

	std::stringstream buffer;
	buffer << emlFile.rdbuf();
	std::string realRawEmail = buffer.str();

	std::cout << "Loaded .eml file. Size: " << realRawEmail.length() << " bytes.\n";

	Email parsedEmail;
	// Парсимо!
	MimeParser::parseEmail(realRawEmail, parsedEmail, systemLogger);

	std::cout << "\n=== PARSED HEADERS ===\n";
	std::cout << "Sender:  " << parsedEmail.sender << "\n";
	std::cout << "Subject: " << parsedEmail.subject << "\n"; // Тепер тут має бути кирилиця!
	std::cout << "Date:    " << parsedEmail.date << "\n";

	std::cout << "\n=== ATTACHMENTS ===\n";
	if (parsedEmail.attachments.empty())
	{
		std::cout << "No attachments.\n";
	}
	else
	{
		for (const auto& attach : parsedEmail.attachments)
		{
			std::cout << "[FILE] Name: " << attach.fileName << "\n";
			std::cout << "       Type: " << attach.mimeType << "\n";
			std::cout << "       Size: " << attach.fileSize << " bytes\n";
			std::cout << "       Data: " << (attach.base64Data.empty() ? "EMPTY" : "OK (Base64)") << "\n\n";
		}
	}

	return 0;
}