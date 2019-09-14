#pragma once

#include <Crypto/SecretKey.h>
#include <Wallet/EncryptedSeed.h>
#include <Common/Secure.h>
#include <optional>
#include <vector>
#include <memory>

//
// A container for the HD Wallet's master seed and other sensitive data.
// The information is stored in an encrypted form, and requires the user's password to retrieve any usable data from it.
//
class SeedEncrypter
{
public:
	//
	// Encrypts the wallet seed using the password and a randomly generated salt.
	//
	EncryptedSeed EncryptWalletSeed(const SecureVector& walletSeed, const SecureString& password) const;

	//
	// Decrypts the wallet seed using the password and salt given.
	//
	std::optional<SecureVector> DecryptWalletSeed(const EncryptedSeed& encryptedSeed, const SecureString& password) const;
};