#include "ClientSecureChannel.hpp"

bool ClientSecureChannel::DeriveKeys(const unsigned char* otherKey, const unsigned char* public_key, const unsigned char* private_key)
{
	unsigned char rx[crypto_kx_SESSIONKEYBYTES];
	unsigned char tx[crypto_kx_SESSIONKEYBYTES];

	int derived = crypto_kx_client_session_keys(rx, tx, public_key, private_key, otherKey);

	if (derived != 0)
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "DERIVE_KEYS: failed to derive session keys (CLIENT)");
		return false;
	}

	std::memcpy(m_rxKey, rx, crypto_kx_SESSIONKEYBYTES);
	std::memcpy(m_txKey, tx, crypto_kx_SESSIONKEYBYTES);

	sodium_memzero(rx, sizeof(rx));
	sodium_memzero(tx, sizeof(tx));

	if (m_logger) m_logger->Log(LogLevel::PROD, "DERIVE_KEYS: session keys successfully derived (CLIENT)");
	return true;
}

bool ClientSecureChannel::StartTLS()
{
	unsigned char publicKey[crypto_kx_PUBLICKEYBYTES];
	unsigned char privateKey[crypto_kx_SECRETKEYBYTES];
	crypto_kx_keypair(publicKey, privateKey);

	if (!m_conn.sendRaw(publicKey, sizeof(publicKey)))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "STARTTLS: failed to send client's public key");
		sodium_memzero(privateKey, sizeof(privateKey));
		return false;
	}

	unsigned char serverPublicKey[crypto_kx_PUBLICKEYBYTES];
	if (!m_conn.receiveRaw(serverPublicKey, sizeof(serverPublicKey)))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "STARTTLS: failed to receive server's public key");
		sodium_memzero(privateKey, sizeof(privateKey));
		return false;
	}

	if (!DeriveKeys(serverPublicKey, publicKey, privateKey))
	{
		sodium_memzero(privateKey, sizeof(privateKey));
		return false;
	}
	
	sodium_memzero(privateKey, sizeof(privateKey));
	if (m_logger) m_logger->Log(LogLevel::PROD, "STARTTLS: TLS handshake succeeded. (CLIENT)");
	return enableSecure();
}
