#pragma once

#include "IConnection.hpp"

#include <boost/asio.hpp>
#include <gtest/gtest_prod.h>
#include <sodium.h>

#include <array>
#include <string>

struct EphemeralKeys
{
	std::array<unsigned char, crypto_kx_PUBLICKEYBYTES> pk;
	std::array<unsigned char, crypto_kx_SECRETKEYBYTES> sk;
};

using DeriveSessionKeys = int (*)(unsigned char* rx, unsigned char* tx,
                                  const unsigned char* pk, const unsigned char* sk,
                                  const unsigned char* other_pk);

class SecureConnection : public IConnection
{
public:
	static constexpr std::size_t K_MAX_PLAINTEXT = 16384;

	explicit SecureConnection(boost::asio::ip::tcp::socket socket);

	void setTimeout(int seconds);
	bool handshake(DeriveSessionKeys derive, const EphemeralKeys& keys);

	bool send(const std::string& data) override;
	bool receive(std::string& out_data) override;
	bool close() override;
	bool isConnected() const noexcept override;

private:
	FRIEND_TEST(SecureConnectionTest, HandshakeProducesMatchingKeys);
	FRIEND_TEST(SecureConnectionTest, SendReceiveRoundtrip);

	bool waitForEvent(bool for_read);
	bool sendRaw(const unsigned char* data, std::size_t len);
	bool recvRaw(unsigned char* data, std::size_t len);

	boost::asio::ip::tcp::socket m_socket;
	int m_timeout_seconds{30};

	std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> m_rx_key{};
	std::array<unsigned char, crypto_kx_SESSIONKEYBYTES> m_tx_key{};
};
