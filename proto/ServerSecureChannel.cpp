#include "ServerSecureChannel.hpp"

bool ServerSecureChannel::DeriveKeys(const unsigned char* otherKey, const unsigned char* public_key, const unsigned char* private_key)
{
	unsigned char rx[crypto_kx_SESSIONKEYBYTES];
	unsigned char tx[crypto_kx_SESSIONKEYBYTES];

	int derived = crypto_kx_server_session_keys(rx, tx, public_key, private_key, otherKey);

	if (derived != 0)
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "DERIVE_KEYS: failed to derive session keys (SERVER)");
		return false;
	}

	std::memcpy(m_rxKey, rx, crypto_kx_SESSIONKEYBYTES);
	std::memcpy(m_txKey, tx, crypto_kx_SESSIONKEYBYTES);

	sodium_memzero(rx, sizeof(rx));
	sodium_memzero(tx, sizeof(tx));

	if (m_logger) m_logger->Log(LogLevel::PROD, "DERIVE_KEYS: session keys successfully derived (SERVER)");
	return true;
}

bool ServerSecureChannel::StartTLS()
{
	unsigned char clientPublicKey[crypto_kx_PUBLICKEYBYTES];

	if (!m_conn.ReceiveRaw(clientPublicKey, sizeof(clientPublicKey)))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "STARTTLS: failed to receive client's public key");
		return false;
	}
	
	unsigned char publicKey[crypto_kx_PUBLICKEYBYTES];
	unsigned char privateKey[crypto_kx_SECRETKEYBYTES];
	crypto_kx_keypair(publicKey, privateKey);

	if (!DeriveKeys(clientPublicKey, publicKey, privateKey))
	{
		sodium_memzero(privateKey, sizeof(privateKey));
		return false;
	}
	
	if (!m_conn.SendRaw(publicKey, sizeof(publicKey)))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "STARTTLS: failed to send server's public key");
		sodium_memzero(privateKey, sizeof(privateKey));
		return false;
	}

	sodium_memzero(privateKey, sizeof(privateKey));
	if (m_logger) m_logger->Log(LogLevel::DEBUG, "STARTTLS: TLS handshake succeeded. (SERVER)");
	return enableSecure();
}
