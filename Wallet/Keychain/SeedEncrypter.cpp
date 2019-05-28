#include "SeedEncrypter.h"

#include <Crypto/Crypto.h>
#include <Crypto/CryptoException.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/HexUtil.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Infrastructure/Logger.h>

std::optional<SecureVector> SeedEncrypter::DecryptWalletSeed(const EncryptedSeed& encryptedSeed, const SecureString& password) const
{
	try
	{
		LoggerAPI::LogDebug("SeedEncrypter::DecryptWalletSeed - Hashing password.");
		SecretKey passwordHash = Crypto::PBKDF(password, encryptedSeed.GetSalt().GetData(), encryptedSeed.GetScryptParameters());

		LoggerAPI::LogDebug("SeedEncrypter::DecryptWalletSeed - Decrypting with AES256.");
		const SecureVector decrypted = Crypto::AES256_Decrypt(encryptedSeed.GetEncryptedSeedBytes(), passwordHash, encryptedSeed.GetIV());

		SecureVector walletSeed(decrypted.begin(), decrypted.begin() + decrypted.size() - 32);

		const CBigInteger<32> hash256 = Crypto::HMAC_SHA256((const std::vector<unsigned char>&)walletSeed, passwordHash.GetBytes().GetData());
		const CBigInteger<32> hash256Check(&decrypted[walletSeed.size()]);

		LoggerAPI::LogDebug("SeedEncrypter::DecryptWalletSeed - Comparing HMAC.");
		if (hash256 == hash256Check)
		{
			LoggerAPI::LogDebug("SeedEncrypter::DecryptWalletSeed - HMAC valid.");
			return std::make_optional<SecureVector>(std::move(walletSeed));
		}
		else
		{
			LoggerAPI::LogError("SeedEncrypter::DecryptWalletSeed - HMAC invalid.");
		}
	}
	catch (CryptoException&)
	{
		LoggerAPI::LogError("SeedEncrypter::DecryptWalletSeed - Crypto exception occurred.");
		return std::nullopt;
	}

	return std::nullopt;
}

EncryptedSeed SeedEncrypter::EncryptWalletSeed(const SecureVector& walletSeed, const SecureString& password) const
{
	CBigInteger<32> randomNumber = RandomNumberGenerator::GenerateRandom32();
	CBigInteger<16> iv = CBigInteger<16>(&randomNumber.GetData()[0]);
	CBigInteger<8> salt(std::vector<unsigned char>(randomNumber.GetData().cbegin() + 16, randomNumber.GetData().cbegin() + 24));

	ScryptParameters parameters(32768, 8, 1);
	SecretKey passwordHash = Crypto::PBKDF(password, salt.GetData(), parameters);

	const CBigInteger<32> hash256 = Crypto::HMAC_SHA256((const std::vector<unsigned char>&)walletSeed, passwordHash.GetBytes().GetData());
	const std::vector<unsigned char>& hash256Bytes = hash256.GetData();

	SecureVector seedPlusHash;
	seedPlusHash.insert(seedPlusHash.begin(), walletSeed.cbegin(), walletSeed.cend());
	seedPlusHash.insert(seedPlusHash.end(), hash256Bytes.cbegin(), hash256Bytes.cend());

	std::vector<unsigned char> encrypted = Crypto::AES256_Encrypt(seedPlusHash, passwordHash, iv);

	return EncryptedSeed(std::move(iv), std::move(salt), std::move(encrypted), std::move(parameters));
}