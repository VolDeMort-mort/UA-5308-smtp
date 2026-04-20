#include <iostream>
#include <regex>
#include <string>

#include "DataBaseManager.h"
#include "Repository/UserRepository.h"
#include "schema.h"

namespace
{
std::string normalizeRoot(std::string root)
{
	while (!root.empty() && (root.back() == '/' || root.back() == '\\'))
		root.pop_back();
	if (!root.empty()) root.push_back('/');
	return root;
}
} // namespace

int main(int argc, char** argv)
{
	std::string root = "./";
	if (argc >= 2 && argv[1] != nullptr) root = argv[1];
	root = normalizeRoot(root);

	std::cout << "Searching for stress test users...\n";

	const std::string dbPath = root + "data/mail.db";
	DataBaseManager db(dbPath, initSchema());
	if (!db.isConnected())
	{
		std::cerr << "Failed to connect to DB\n";
		return 1;
	}

	UserRepository repo(db);

	// pattern: user followed by digits only e.g. user1, user42, user100
	const std::regex stressUserPattern("^stress_user[0-9]+$");

	int deleted = 0;
	int offset = 0;
	const int batchSize = 100;

	while (true)
	{
		auto batch = repo.findAll(batchSize, offset);
		if (batch.empty()) break;

		for (const auto& user : batch)
		{
			if (std::regex_match(user.username, stressUserPattern))
			{
				if (repo.hardDelete(user.id.value()))
				{
					std::cout << "Deleted: " << user.username << "\n";
					deleted++;
				}
				else
				{
					std::cerr << "Failed to delete: " << user.username << "\n";
				}
			}
		}

		// if we got less than batchSize, we reached the end
		if ((int)batch.size() < batchSize) break;

		offset += batchSize;
	}

	std::cout << "Done! Deleted " << deleted << " stress test users\n";
	return 0;
}