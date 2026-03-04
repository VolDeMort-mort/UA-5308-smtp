#pragma once

#include "SecureChannel.hpp"

class ClientSecureChannel : public SecureChannel
{
public:
	ClientSecureChannel(SocketConnection& conn) : SecureChannel(conn) {};

	bool StartTLS() override;
	bool DeriveKeys(const unsigned char* otherKey, const unsigned char* public_key,
					const unsigned char* private_key) override;
};
