#include "SeedEncrypter.h"

#include <Wallet/Exceptions/KeyChainException.h>
#include <Crypto/AES256.h>
#include <Crypto/KDF.h>
#include <Crypto/Hasher.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>

SecureVector SeedEncrypter::DecryptWalletSeed(const EncryptedSeed& encryptedSeed, const SecureString& password)
{
	try
	{
		WALLET_INFO("Decrypting wallet seed");
		SecretKey passwordHash = KDF::PBKDF(
			password,
			encryptedSeed.GetSalt().GetData(),
			encryptedSeed.GetScryptParameters()
		);

		const SecureVector decrypted = AES256::Decrypt(
			encryptedSeed.GetEncryptedSeedBytes(),
			passwordHash,
			encryptedSeed.GetIV()
		);

		SecureVector walletSeed(decrypted.begin(), decrypted.begin() + decrypted.size() - 32);

		Hash hash256 = Hasher::HMAC_SHA256(
			walletSeed.data(),
			walletSeed.size(),
			passwordHash.data(),
			passwordHash.size()
		);
		Hash hash256Check(decrypted.data() + walletSeed.size());

		if (hash256 == hash256Check) {
			return walletSeed;
		}
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Exception thrown: {}", e.what());
	}

	throw KEYCHAIN_EXCEPTION("Failed to decrypt seed.");
}

EncryptedSeed SeedEncrypter::EncryptWalletSeed(const SecureVector& walletSeed, const SecureString& password)
{
	WALLET_INFO("Encrypting wallet seed");

	CBigInteger<16> iv(CSPRNG::GenerateRandomBytes(16).data());
	CBigInteger<8> salt(CSPRNG::GenerateRandomBytes(8).data());

	ScryptParameters parameters(32768, 8, 1);
	SecretKey passwordHash = KDF::PBKDF(password, salt.GetData(), parameters);

	Hash hash256 = Hasher::HMAC_SHA256(
		walletSeed.data(),
		walletSeed.size(),
		passwordHash.data(),
		passwordHash.size()
	);

	SecureVector seedPlusHash;
	seedPlusHash.insert(seedPlusHash.begin(), walletSeed.cbegin(), walletSeed.cend());
	seedPlusHash.insert(seedPlusHash.end(), hash256.GetData().cbegin(), hash256.GetData().cend());

	std::vector<uint8_t> encrypted = AES256::Encrypt(seedPlusHash, passwordHash, iv);

	return EncryptedSeed(std::move(iv), std::move(salt), std::move(encrypted), std::move(parameters));
}