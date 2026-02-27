#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "../common/models/Email.h"
#include "../common/mime/MimeParser.h"
#include "MockLogger.h" 

int main() {
    std::cout << "=== REAL .eml MIME CRASH TEST ===\n\n";

    MockLogger systemLogger;

    std::ifstream emlFile("C:/Users/WellDone/Downloads/emlFile.eml");

    if (!emlFile.is_open()) {
        std::cerr << "Error: Could not open the .eml file!\n";
        return 1;
    }

    std::stringstream buffer;
    buffer << emlFile.rdbuf();
    std::string realRawEmail = buffer.str();

    std::cout << "Loaded .eml file. Size: " << realRawEmail.length() << " bytes.\n";
    std::cout << "Starting parser...\n\n";

    Email parsedEmail;
    bool parseSuccess = MimeParser::parseEmail(realRawEmail, parsedEmail, systemLogger);

    if (!parseSuccess) {
        std::cerr << "CRASH! Failed to parse the real email!\n";
        return 1;
    }

    std::cout << "\n=== PARSED RESULTS ===\n";
    std::cout << "Sender: " << parsedEmail.sender << "\n";
    std::cout << "Subject: " << parsedEmail.subject << "\n";

    std::cout << "\nPlain Text length: " << parsedEmail.plainText.length() << " chars\n";
    if (parsedEmail.plainText.length() > 0) {
        std::cout << "Preview: " << parsedEmail.plainText.substr(0, 50) << "...\n";
    }

    std::cout << "\nHTML Text length: " << parsedEmail.htmlText.length() << " chars\n";
    std::cout << "Forwarded emails count: " << parsedEmail.forwardedEmails.size() << "\n";
    std::cout << "Attachments count: " << parsedEmail.attachments.size() << "\n";

    return 0;
}