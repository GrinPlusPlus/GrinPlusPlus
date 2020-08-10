#pragma once

#include <Crypto/Models/ed25519_public_key.h>
#include <Crypto/Models/ed25519_secret_key.h>

struct ed25519_keypair_t
{
	ed25519_keypair_t() = default;
	ed25519_keypair_t(ed25519_public_key_t&& public_key_, ed25519_secret_key_t&& secret_key_)
		: public_key(std::move(public_key_)), secret_key(std::move(secret_key_)) { }

	ed25519_public_key_t public_key;
	ed25519_secret_key_t secret_key;
};