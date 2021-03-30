#pragma once

#include <Common/Secure.h>
#include <Crypto/Models/SecretKey.h>
#include <optional>

// Forward Declarations
struct ScryptParameters;

class KDF
{
public:
	//
	// Uses Scrypt to hash the given password and the given salt. It then blake2b hashes the output.
	// The returned hash will be a 32 byte SecretKey.
	//
	static SecretKey PBKDF(
		const SecureString& password,
		const std::vector<uint8_t>& salt,
		const ScryptParameters& parameters
	);

	static SecretKey HKDF(
		const std::optional<std::vector<uint8_t>>& saltOpt,
		const std::string& label,
		const std::vector<uint8_t>& inputKeyingMaterial
	);
};