#pragma once

#include <Wallet/WalletDB/Models/EncryptedSeed.h>
#include <Common/Secure.h>

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
	static EncryptedSeed EncryptWalletSeed(const SecureVector& walletSeed, const SecureString& password);

	//
	// Decrypts the wallet seed using the password and salt given.
	//
	// Throws KeyChainException if decryption fails.
	//
	static SecureVector DecryptWalletSeed(const EncryptedSeed& encryptedSeed, const SecureString& password);
};