#include "Config.h"
#include "JsonParser.h"

#include <stdexcept>


namespace SmtpClient {

// Helpers

namespace {

int ToInt(const JsonParser::FlatMap& map, const std::string& key, int default_val)
{
	auto it = map.find(key);
	if (it == map.end() || it->second.empty())
		return default_val;
	try { return std::stoi(it->second); }
	catch (...) { return default_val; }
}

std::string ToString(const JsonParser::FlatMap& map,
                     const std::string& key,
                     const std::string& default_val)
{
	auto it = map.find(key);
	if (it == map.end() || it->second.empty())
		return default_val;
	return it->second;
}

uint16_t ToUint16(const JsonParser::FlatMap& map, const std::string& key, uint16_t default_val)
{
	auto it = map.find(key);
	if (it == map.end() || it->second.empty())
		return default_val;
	try
	{
		int v = std::stoi(it->second);
		// check if port is real
		if (v < 0 || v > 65535) return default_val;
		return static_cast<uint16_t>(v);
	}
	catch (...) { return default_val; }
}

} // anonymous namespace


// Singleton
Config& Config::Instance()
{
	static Config instance;
	return instance;
}


bool Config::Load(const std::string& path)
{
	const auto map = JsonParser::ParseFile(path);
	if (map.empty())
		return false;

	// server
	m_config.server.port = ToUint16(map, "server.port", m_config.server.port);
	m_config.server.domain = ToString(map, "server.domain", m_config.server.domain);
	m_config.server.worker_threads = ToInt (map, "server.worker_threads", m_config.server.worker_threads);
	m_config.server.db_path = ToString(map, "server.db_path", m_config.server.db_path);
	m_config.server.migration_path = ToString(map, "server.migration_path", m_config.server.migration_path);

	// imap
	m_config.imap.port = ToUint16(map, "imap.port", m_config.imap.port);
	m_config.imap.db_path = ToString(map, "imap.db_path", m_config.imap.db_path);
	m_config.imap.migration_path = ToString(map, "imap.migration_path", m_config.imap.migration_path);
	m_config.imap.timeout_mins = ToInt (map, "imap.timeout_mins", m_config.imap.timeout_mins);
    m_config.imap.worker_threads = ToInt(map, "imap.worker_threads", m_config.imap.worker_threads);
    m_config.imap.handshake_timeout_secs = ToInt(map, "imap.handshake_timeout_secs", m_config.imap.handshake_timeout_secs);

	// logging
	m_config.logging.log_level = ToString(map, "logging.log_level", m_config.logging.log_level);
	m_config.logging.file_path = ToString(map, "logging.file_path", m_config.logging.file_path);
	m_config.logging.old_file_path = ToString(map, "logging.old_file_path", m_config.logging.old_file_path);
	m_config.logging.max_file_size_mb = ToInt (map, "logging.max_file_size_mb", m_config.logging.max_file_size_mb);
	m_config.logging.batch_size = ToInt (map, "logging.batch_size", m_config.logging.batch_size);
	m_config.logging.flush_interval_secs = ToInt (map, "logging.flush_interval_secs", m_config.logging.flush_interval_secs);

	// proto
	m_config.proto.max_line_size = ToInt(map, "proto.max_line_size", m_config.proto.max_line_size);
	m_config.proto.socket_timeout_secs = ToInt(map, "proto.socket_timeout_secs", m_config.proto.socket_timeout_secs);
	m_config.proto.max_message_size_mb = ToInt(map, "proto.max_message_size_mb", m_config.proto.max_message_size_mb);

	// database
	m_config.database.default_page_limit = ToInt(map, "database.default_page_limit", m_config.database.default_page_limit);

	// mime
	m_config.mime.header_chunk_size = ToInt (map, "mime.header_chunk_size", m_config.mime.header_chunk_size);
	m_config.mime.default_domain = ToString(map, "mime.default_domain",    m_config.mime.default_domain);

	// base64
	m_config.base64.max_file_size_mb = ToInt(map, "base64.max_file_size_mb", m_config.base64.max_file_size_mb);
	m_config.base64.chunk_size_bytes = ToInt(map, "base64.chunk_size_bytes", m_config.base64.chunk_size_bytes);
	m_config.base64.line_length  = ToInt(map, "base64.line_length",      m_config.base64.line_length);

	return true;
}

} // namespace SmtpClient
