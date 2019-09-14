#pragma once

#include <Crypto/SecretKey.h>
#include <Common/Secure.h>
#include <string>
#include <vector>

class WalletEncryptionUtil
{
public:
	static std::vector<unsigned char> Encrypt(const SecureVector& masterSeed, const std::string& dataType, const SecureVector& bytes);
	static SecureVector Decrypt(const SecureVector& masterSeed, const std::string& dataType, const std::vector<unsigned char>& encrypted);

private:
	static SecretKey CreateSecureKey(const SecureVector& masterSeed, const std::string& dataType);
};