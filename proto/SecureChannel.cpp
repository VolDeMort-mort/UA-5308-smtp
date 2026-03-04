#include "SecureChannel.hpp"

bool SecureChannel:: isSecure() const
{
	return m_secure;
}

void SecureChannel::setLogger(Logger* logger)
{
	m_logger = logger;
}

std::string SecureChannel:: Encrypt(const std::string raw_data)
{
	unsigned char nonce[crypto_aead_chacha20poly1305_ietf_NPUBBYTES];
	randombytes_buf(nonce, sizeof(nonce));

	std::string encrypted_text;
	encrypted_text.resize(crypto_aead_chacha20poly1305_ietf_ABYTES + raw_data.size());

	unsigned long long encrypted_text_len = 0;

	int encrypt = crypto_aead_chacha20poly1305_ietf_encrypt(
		reinterpret_cast<unsigned char*>(&encrypted_text[0]), &encrypted_text_len,
		reinterpret_cast<const unsigned char*>(raw_data.data()), raw_data.size(), nullptr, 0, nullptr, nonce, m_txKey);

	if (encrypt != 0)
	{
		if(m_logger) m_logger->Log(LogLevel::PROD, "Encryption failed");
		return {};
	}

	encrypted_text.resize(encrypted_text_len);

	std::string pair;
	pair.reserve(sizeof(nonce) + encrypted_text.size());
	pair.append(reinterpret_cast<char*>(nonce), sizeof(nonce));
	pair.append(encrypted_text);

	return pair;
}

std::string SecureChannel::Decrypt(const std::string raw_data)
{
	const std::size_t nonce_size = crypto_aead_chacha20poly1305_ietf_NPUBBYTES;
	const std::size_t tag_size = crypto_aead_chacha20poly1305_ietf_ABYTES;

	if (raw_data.size() < nonce_size + tag_size)
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "Encrypted text too short");
		return {};
	}

	unsigned char nonce[nonce_size] = {};
	std::memcpy(nonce, raw_data.data(), nonce_size);

	const unsigned char* encrypted_text = reinterpret_cast<const unsigned char*>(raw_data.data() + nonce_size);
	std::size_t encrypted_text_len = raw_data.size() - nonce_size;

	std::string decrypted_text;

	if (encrypted_text_len < tag_size)
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "Encrypted data too short for tag");
		return {};
	}

	decrypted_text.resize(encrypted_text_len - tag_size);
	unsigned long long decrypted_text_len = 0;

	int decrypt = crypto_aead_chacha20poly1305_ietf_decrypt(reinterpret_cast<unsigned char*>(&decrypted_text[0]),
															&decrypted_text_len, nullptr, encrypted_text,
															encrypted_text_len, nullptr, 0, nonce, m_rxKey);

	if (decrypt != 0)
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "Decryption failed");
		return {};
	}

	decrypted_text.resize(decrypted_text_len);

	return decrypted_text;
}

bool SecureChannel::enableSecure()
{
	m_secure = true;
	return true;
}

bool SecureChannel::Send(const std::string& data)
{
	if (!m_secure)
	{
		return m_conn.send(data);
	}

	std::string encrypted_text = Encrypt(data);
	std::uint32_t text_len = static_cast<std::uint32_t>(encrypted_text.size());
	std::uint32_t net_len = htonl(text_len);

	if (!m_conn.sendRaw(reinterpret_cast<unsigned char*>(&net_len), sizeof(net_len)))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "SECURECHANNEL_SEND: failed to send message length");
		return false;
	}

	if (!m_conn.sendRaw(reinterpret_cast<const unsigned char*>(encrypted_text.data()), encrypted_text.size()))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "SECURECHANNEL_SEND: failed to send message");
		return false;
	}

	if (m_logger) m_logger->Log(LogLevel::TRACE, "SECURECHANNEL_SEND: sent " + std::to_string(data.size()) + " bytes");
	return true;
}

bool SecureChannel::Receive(std::string& data)
{
	if (!m_secure)
	{
		return m_conn.receive(data);
	}

	std::uint32_t text_len = 0;

	if (!m_conn.receiveRaw(reinterpret_cast<unsigned char*>(&text_len), sizeof(text_len)))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "SECURECHANNEL_RECEIVE: failed to receive message length");
		return false;
	}

	text_len = ntohl(text_len);

	std::string encrypted_text(text_len, '\0');
	if (!m_conn.receiveRaw(reinterpret_cast<unsigned char*>(&encrypted_text[0]), text_len))
	{
		if (m_logger) m_logger->Log(LogLevel::PROD, "SECURECHANNEL_RECEIVE: failed to receive message");
		return false;
	}

	data = Decrypt(encrypted_text);
	if (!data.empty() && data.back() == '\n') data.pop_back();
	if (!data.empty() && data.back() == '\r') data.pop_back();

	if (m_logger) m_logger->Log(LogLevel::TRACE, "Receive: received " + std::to_string(data.size()) + " bytes");
	return true;
}
