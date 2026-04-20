#pragma once

#include "SecureChannel.hpp"

#include <sodium.h>

class ServerSecureChannel : public SecureChannel
{
public:
	ServerSecureChannel(IConnection& conn) : SecureChannel(conn) {};

	bool StartTLS() override;
	bool DeriveKeys(const unsigned char* otherKey, const unsigned char* public_key,
					const unsigned char* private_key) override;
};
