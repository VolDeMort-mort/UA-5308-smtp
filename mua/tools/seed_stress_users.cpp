#include <iostream>
#include <optional>
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

bool ensureUser(UserRepository& repo, const std::string& username, const std::string& password)
{
	auto existing = repo.findByUsername(username);
	if (!existing.has_value())
	{
		User user;
		user.username = username;
		user.first_name = std::nullopt;
		user.last_name = std::nullopt;
		user.birthdate = std::nullopt;
		user.avatar_b64 = std::nullopt;
		return repo.registerUser(user, password);
	}
	return repo.changePassword(existing->id.value(), password);
}
} // namespace

int main(int argc, char** argv)
{
	std::string root = "./";
	int count = 50; // default

	if (argc >= 2 && argv[1] != nullptr) root = argv[1];
	if (argc >= 3 && argv[2] != nullptr) count = std::stoi(argv[2]);

	root = normalizeRoot(root);

	std::cout << "Seeding " << count << " stress test users...\n";

	const std::string dbPath = root + "data/mail.db";
	DataBaseManager db(dbPath, initSchema());
	if (!db.isConnected())
	{
		std::cerr << "Failed to connect to DB\n";
		return 1;
	}

	UserRepository repo(db);
	bool ok = true;

	for (int i = 1; i <= count; i++)
	{
		std::string username = "stress_user" + std::to_string(i);
		if (!ensureUser(repo, username, "pass"))
		{
			std::cerr << "Failed to create user: " << username << "\n";
			ok = false;
		}
	}

	if (ok) std::cout << "Done! Created " << count << " users (stress_user1..stress_user" << count << "), password: pass\n";

	return ok ? 0 : 1;
}