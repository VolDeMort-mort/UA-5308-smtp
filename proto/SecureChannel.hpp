#pragma once

#include "SocketConnection.hpp"
#include "ILogger.h"

#include <sodium.h>
#include <cstdint>

class SecureChannel
{
public:
	SecureChannel(SocketConnection& conn) : m_conn(conn){};
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
	SocketConnection& m_conn;
	ILogger* m_logger = nullptr;

	bool m_secure = false;
	bool enableSecure();

	unsigned char m_txKey[crypto_kx_SESSIONKEYBYTES] = {};
	unsigned char m_rxKey[crypto_kx_SESSIONKEYBYTES] = {};

	virtual bool DeriveKeys(const unsigned char* otherKey, const unsigned char* public_key,
							const unsigned char* private_key) = 0;

private:
	std::string Encrypt(const std::string raw_data);
	std::string Decrypt(const std::string data);
};
