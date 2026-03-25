#pragma once
#include "AppConfig.h"
#include <string>

namespace SmtpClient {

/**
 * Usage:
 *   // At program startup (main):
 *   Config::Instance().Load("config.json");
 *
 *   // Anywhere in the codebase:
 *   uint16_t port = Config::Instance().GetServer().port;
 *
 * If Load() is never called, all sections return their default values
 * as defined in AppConfig.h
 *
 * Thread-safety: Load() must be called once before any concurrent reads.
 * After initialization the object is read-only and safe to use from
 * multiple threads.
 */
class Config
{
public:
	static Config& Instance();

	/**
	 * @brief Load configuration from a JSON file.
	 * @param path  Path to the .json file.
	 * @return true on success, false if the file cannot be opened or parsed.
	 *
	 * Can be called multiple times (e.g. in tests) to reload configuration.
	 * On failure the previously loaded (or default) values are preserved.
	 */
	bool Load(const std::string& path);

	const ServerConfig& GetServer() const { return m_config.server;   }
	const ImapConfig& GetImap() const { return m_config.imap;     }
	const LoggingConfig& GetLogging() const { return m_config.logging;  }
	const ProtoConfig& GetProto() const { return m_config.proto;    }
	const DatabaseConfig& GetDatabase() const { return m_config.database; }
	const MimeConfig& GetMime() const { return m_config.mime;     }
	const Base64Config& GetBase64() const { return m_config.base64;   }

private:
	Config() = default;
	Config(const Config&) = delete;
	Config& operator=(const Config&) = delete;

	AppConfig m_config;
};

} // namespace SmtpClient
