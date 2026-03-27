#include "ImapConfig.hpp"

#include <stdexcept>
#include <string>

bool ImapConfig::Parse(const mINI::INIStructure& ini, ILogger& logger)
{
	if (!ini.has("imap"))
	{
		logger.Log(PROD, "Couldn't find [imap] section in config - using default values");
		return false;
	}

	bool all_found = true;
	const auto& imap_vals = ini.get("imap");

	auto log = [&logger, &all_found](const bool res, const std::string& var_name)
	{
		if (res)
		{
			logger.Log(DEBUG, var_name + " key was successfully found and parsed into config");
		}
		else
		{
			all_found = false;
			logger.Log(PROD, var_name + " key wasn`t found/parsed");
		}
	};

	log(ParseIntField(imap_vals, "port", PORT), "port");
	log(ParseIntField(imap_vals, "timeout_mins", TIMEOUT_MINS), "timeout_mins");
	log(ParseIntField(imap_vals, "worker_threads", WORKER_THREADS), "worker_threads");
	log(ParseIntField(imap_vals, "handshake_timeout_secs", HANDSHAKE_TIMEOUT_SECS), "handshake_timeout_secs");

	return all_found;
}

bool ImapConfig::ParseIntField(const mINI::INIMap<std::string>& section, const std::string& key, int& target_field)
{
	if (!section.has(key))
	{
		return false;
	}

	try
	{
		target_field = std::stoi(section.get(key));
	}
	catch (const std::exception& e)
	{
		return false;
	}

	return true;
}