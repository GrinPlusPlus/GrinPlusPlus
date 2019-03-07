#include "SeedEncrypter.h"

#include <Crypto/Crypto.h>
#include <Crypto/CryptoException.h>
#include <Common/Util/VectorUtil.h>
#include <Common/Util/HexUtil.h>
#include <Crypto/RandomNumberGenerator.h>

std::optional<CBigInteger<32>> SeedEncrypter::DecryptWalletSeed(const EncryptedSeed& encryptedSeed, const SecureString& password) const
{
	try
	{
		CBigInteger<32> passwordHash = CalculatePasswordHash(password, encryptedSeed.GetSalt());
		const std::vector<unsigned char> decrypted = Crypto::AES256_Decrypt(encryptedSeed.GetEncryptedSeedBytes(), passwordHash, encryptedSeed.GetIV());
		if (decrypted.size() != 64)
		{
			return std::nullopt;
		}

		CBigInteger<32> walletSeed(&decrypted[0]);

		const CBigInteger<32> hash256 = Crypto::HMAC_SHA256(walletSeed.GetData(), passwordHash.GetData());
		const CBigInteger<32> hash256Check(&decrypted[32]);
		if (hash256 == hash256Check)
		{
			return std::make_optional<CBigInteger<32>>(std::move(walletSeed));
		}
	}
	catch (CryptoException&)
	{
		return std::nullopt;
	}

	return std::nullopt;
}

EncryptedSeed SeedEncrypter::EncryptWalletSeed(const CBigInteger<32>& walletSeed, const SecureString& password) const
{
	CBigInteger<32> randomNumber = RandomNumberGenerator::GenerateRandom32();
	CBigInteger<16> iv = CBigInteger<16>(&randomNumber.GetData()[0]);
	CBigInteger<8> salt(std::vector<unsigned char>(randomNumber.GetData().cbegin() + 16, randomNumber.GetData().cbegin() + 24));

	CBigInteger<32> passwordHash = CalculatePasswordHash(password, salt);

	const std::vector<unsigned char>& walletSeedBytes = walletSeed.GetData();

	const CBigInteger<32> hash256 = Crypto::HMAC_SHA256(walletSeedBytes, passwordHash.GetData());
	const std::vector<unsigned char>& hash256Bytes = hash256.GetData();

	std::vector<unsigned char> seedPlusHash;
	seedPlusHash.insert(seedPlusHash.begin(), walletSeedBytes.cbegin(), walletSeedBytes.cend());
	seedPlusHash.insert(seedPlusHash.end(), hash256Bytes.cbegin(), hash256Bytes.cend());

	std::vector<unsigned char> encrypted = Crypto::AES256_Encrypt(seedPlusHash, passwordHash, iv);

	return EncryptedSeed(std::move(iv), std::move(salt), std::move(encrypted));
}

CBigInteger<32> SeedEncrypter::CalculatePasswordHash(const SecureString& password, const CBigInteger<8>& salt) const
{
	const std::vector<unsigned char> passwordBytes(&password[0], &password[0] + password.length());
	const CBigInteger<64> scryptHash = Crypto::Scrypt(passwordBytes, salt.GetData());
	return Crypto::Blake2b(scryptHash.GetData());
}