#pragma once

#include <mini/ini.h>

#include "ILogger.h"

struct ImapConfig
{
public:
	int PORT = 2553;
	int TIMEOUT_MINS = 30;
	int WORKER_THREADS = 4;

	// returns true if parsed successfully fully, otherwise - false
	bool Parse(const mINI::INIStructure& ini, ILogger& logger);

private:
	// returns true if intField was found and parsed successfully
	bool ParseIntField(const mINI::INIMap<std::string>& section, const std::string& key, int& target_field);
};