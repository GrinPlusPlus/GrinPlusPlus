#include "SeedEncrypter.h"

#include <Wallet/Exceptions/KeyChainException.h>
#include <Crypto/Crypto.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Common/Logger.h>

SecureVector SeedEncrypter::DecryptWalletSeed(const EncryptedSeed& encryptedSeed, const SecureString& password) const
{
	try
	{
		WALLET_INFO("Decrypting wallet seed");
		SecretKey passwordHash = Crypto::PBKDF(password, encryptedSeed.GetSalt().GetData(), encryptedSeed.GetScryptParameters());

		const SecureVector decrypted = Crypto::AES256_Decrypt(encryptedSeed.GetEncryptedSeedBytes(), passwordHash, encryptedSeed.GetIV());

		SecureVector walletSeed(decrypted.begin(), decrypted.begin() + decrypted.size() - 32);

		const CBigInteger<32> hash256 = Crypto::HMAC_SHA256((const std::vector<unsigned char>&)walletSeed, passwordHash.GetVec());
		const CBigInteger<32> hash256Check(&decrypted[walletSeed.size()]);

		if (hash256 == hash256Check)
		{
			return walletSeed;
		}
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Exception thrown: {}", e.what());
	}

	throw KEYCHAIN_EXCEPTION("Failed to decrypt seed.");
}

EncryptedSeed SeedEncrypter::EncryptWalletSeed(const SecureVector& walletSeed, const SecureString& password) const
{
	WALLET_INFO("Encrypting wallet seed");

	CBigInteger<32> randomNumber = RandomNumberGenerator::GenerateRandom32();
	CBigInteger<16> iv = CBigInteger<16>(&randomNumber.GetData()[0]);
	CBigInteger<8> salt(std::vector<unsigned char>(randomNumber.GetData().cbegin() + 16, randomNumber.GetData().cbegin() + 24));

	ScryptParameters parameters(32768, 8, 1);
	SecretKey passwordHash = Crypto::PBKDF(password, salt.GetData(), parameters);

	const CBigInteger<32> hash256 = Crypto::HMAC_SHA256((const std::vector<unsigned char>&)walletSeed, passwordHash.GetVec());
	const std::vector<unsigned char>& hash256Bytes = hash256.GetData();

	SecureVector seedPlusHash;
	seedPlusHash.insert(seedPlusHash.begin(), walletSeed.cbegin(), walletSeed.cend());
	seedPlusHash.insert(seedPlusHash.end(), hash256Bytes.cbegin(), hash256Bytes.cend());

	std::vector<unsigned char> encrypted = Crypto::AES256_Encrypt(seedPlusHash, passwordHash, iv);

	return EncryptedSeed(std::move(iv), std::move(salt), std::move(encrypted), std::move(parameters));
}