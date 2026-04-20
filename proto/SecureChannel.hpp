#pragma once

#include "IConnection.hpp"
#include "Logger.h"
#include "SocketConnection.hpp"

#include <sodium.h>
#include <cstdint>
#include <optional>

class SecureChannel
{
public:
	SecureChannel(IConnection& conn) : m_conn(conn){};
	virtual ~SecureChannel() 
	{
		sodium_memzero(m_txKey, sizeof(m_txKey));
		sodium_memzero(m_rxKey, sizeof(m_rxKey));
	};

	bool Send(const std::string& data);
	bool Receive(std::string& data);

	virtual bool StartTLS() = 0;
	
	bool isSecure() const;
	void setLogger(ILogger* logger);

protected:
	IConnection& m_conn;
	ILogger* m_logger = nullptr;

	bool m_secure = false;
	bool enableSecure();

	unsigned char m_txKey[crypto_kx_SESSIONKEYBYTES] = {};
	unsigned char m_rxKey[crypto_kx_SESSIONKEYBYTES] = {};

	virtual bool DeriveKeys(const unsigned char* otherKey, const unsigned char* public_key,
							const unsigned char* private_key) = 0;

private:
	static constexpr std::uint32_t MAX_MESSAGE_SIZE = 10 * 1024 * 1024; // plan to use config value

	std::uint64_t m_txSeq = 0;
	std::uint64_t m_rxSeq = 0;

	std::string Encrypt(const std::string& raw_data);
	std::optional<std::string> Decrypt(const std::string& data);
};
