#pragma once

#include <Crypto/Models/SecretKey.h>
#include <Common/Secure.h>
#include <string>
#include <vector>

class WalletEncryptionUtil
{
public:
	static std::vector<uint8_t> Encrypt(
		const SecureVector& masterSeed,
		const std::string& dataType,
		const SecureVector& bytes
	);

	static SecureVector Decrypt(
		const SecureVector& masterSeed,
		const std::string& dataType,
		const std::vector<uint8_t>& encrypted
	);

private:
	static SecretKey CreateSecureKey(const SecureVector& masterSeed, const std::string& dataType);
};