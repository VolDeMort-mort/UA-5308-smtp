#include "SecureConnection.hpp"

#include <arpa/inet.h>
#include <cstddef>
#include <vector>

SecureConnection::SecureConnection(boost::asio::ip::tcp::socket socket) : m_socket(std::move(socket)) {}

bool SecureConnection::sendRaw(const unsigned char* data, std::size_t len)
{
	boost::system::error_code ec;
	boost::asio::write(m_socket, boost::asio::buffer(data, len), ec);

	return !ec;
}

bool SecureConnection::recvRaw(unsigned char* data, std::size_t len)
{
	boost::system::error_code ec;
	boost::asio::read(m_socket, boost::asio::buffer(data, len), ec);

	return !ec;
}

bool SecureConnection::handshake(DeriveSessionKeys derive, const EphemeralKeys& keys)
{
	if (!sendRaw(keys.pk.data(), crypto_kx_PUBLICKEYBYTES)) return false;

	unsigned char peer_pk[crypto_kx_PUBLICKEYBYTES];
	if (!recvRaw(peer_pk, crypto_kx_PUBLICKEYBYTES)) return false;

	if (derive(m_rx_key.data(), m_tx_key.data(), keys.pk.data(), keys.sk.data(), peer_pk) != 0)
	{
		// provide logger integration to report suspicious key
		return false;
	}

	return true;
}

bool SecureConnection::send(const std::string& data)
{

	auto header_length = sizeof(uint32_t);
	auto payload_length = crypto_secretbox_NONCEBYTES + crypto_secretbox_MACBYTES + data.size();

	/* libsodium operates and writes behind raw pointers, so if we want to use std::vector to hold our encrypted message
	 * we need to opt for constructor that sets vector's size and initialized all elements to a
	 * default state so subsequent call to crypto_secretbox_easy can correctly overwrite them
	 * reserve() would break this
	 */
	std::vector<unsigned char> frame(header_length + payload_length);
	/* std::vector<std::byte> didn't cut it since it is doesn't implictly convert to
	 * unsigned char and libsodium is a C API which expects that
	 * it would require explicit casts on every call site to such API
	 */
	uint32_t header = htonl(payload_length);
	memcpy(frame.data(), &header, header_length);

	randombytes_buf(frame.data() + header_length, crypto_secretbox_NONCEBYTES);

	crypto_secretbox_easy(frame.data() + header_length + crypto_secretbox_NONCEBYTES,
						  reinterpret_cast<const unsigned char*>(data.data()), data.size(),
						  frame.data() + header_length, m_tx_key.data());

	return sendRaw(frame.data(), frame.size());
}

bool SecureConnection::receive(std::string& out_data)
{

	uint32_t net_len;
	recvRaw(reinterpret_cast<unsigned char*>(&net_len), sizeof(net_len));
	uint32_t payload_len = ntohl(net_len);

	unsigned char nonce[crypto_secretbox_NONCEBYTES];
	recvRaw(nonce, crypto_secretbox_NONCEBYTES);

	auto ciphertext_len = payload_len - crypto_secretbox_NONCEBYTES;
	std::vector<unsigned char> ciphertext(ciphertext_len);
	recvRaw(ciphertext.data(), ciphertext_len);

	auto plaintext_len = ciphertext_len - crypto_secretbox_MACBYTES;
	std::vector<unsigned char> plaintext(plaintext_len);
	crypto_secretbox_open_easy(plaintext.data(), ciphertext.data(), ciphertext_len, nonce, m_rx_key.data());

	out_data.assign(reinterpret_cast<const char*>(plaintext.data()), plaintext_len);

	return true;
}

bool SecureConnection::close()
{
	boost::system::error_code ec;

	m_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, ec);
	m_socket.close(ec);

	return true;
}

bool SecureConnection::isConnected() const noexcept
{
	return m_socket.is_open();
}
