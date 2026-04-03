#pragma once
#include <cstdint>
#include <string>

namespace SmtpClient
{

struct ServerConfig
{
	uint16_t port = 25000;
	std::string domain = "localhost";
	int worker_threads = 4;
	std::string db_path = "mail.db";
	std::string migration_path = "../../database/scheme/001_init_scheme.sql";
};

struct ImapConfig
{
	uint16_t port = 2553;
	std::string db_path = "prod.db";
	std::string migration_path = "../../database/scheme/001_init_scheme.sql";
	int timeout_mins = 30;
	int worker_threads = 4;
	int handshake_timeout_secs = 10;
};

struct LoggingConfig
{
	std::string log_level = "PROD"; // NONE / PROD / DEBUG / TRACE
	std::string file_path = "log.txt";
	std::string old_file_path = "old_log.txt";
	int max_file_size_mb = 10;
	int batch_size = 50;
	int flush_interval_secs = 2;
};

struct ProtoConfig
{
	int max_line_size = 8192;
	int socket_timeout_secs = 30;
	int max_message_size_mb = 10;
};

struct DatabaseConfig
{
	int default_page_limit = 50;
};

struct MimeConfig
{
	int header_chunk_size = 45;
	std::string default_domain = "localhost";
};

struct Base64Config
{
	int max_file_size_mb = 15;
	int chunk_size_bytes = 3072;
	int line_length = 76;
};

struct AppConfig
{
	ServerConfig server;
	ImapConfig imap;
	LoggingConfig logging;
	ProtoConfig proto;
	DatabaseConfig database;
	MimeConfig mime;
	Base64Config base64;
};

} // namespace SmtpClient
