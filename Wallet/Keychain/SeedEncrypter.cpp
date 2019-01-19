#include "SeedEncrypter.h"

#include <Crypto.h>
#include <VectorUtil.h>
#include <HexUtil.h>
#include <Crypto/RandomNumberGenerator.h>

CBigInteger<32> SeedEncrypter::DecryptWalletSeed(const EncryptedBytes& encryptedBytes, const Salt& salt, const IV& iv, const std::string& password) const
{
	CBigInteger<32> passwordHash = CalculatePasswordHash(password, salt);
	const std::vector<unsigned char> decrypted = Crypto::AES256_Decrypt(encryptedBytes, passwordHash, iv);
	if (decrypted.size() != 64)
	{
		return CBigInteger<32>();
	}

	const CBigInteger<32> walletSeed(&decrypted[0]);
	const CBigInteger<32> hash256 = Crypto::HMAC_SHA256(walletSeed.GetData(), passwordHash.GetData());

	const CBigInteger<32> hash256Check(&decrypted[32]);

	if (hash256 == hash256Check)
	{
		return walletSeed;
	}

	return CBigInteger<32>();
}

std::tuple<Salt, IV, EncryptedBytes> SeedEncrypter::EncryptWalletSeed(const CBigInteger<32>& walletSeed, const std::string& password) const
{
	CBigInteger<32> randomNumber = RandomNumberGenerator().GeneratePseudoRandomNumber(CBigInteger<32>(), CBigInteger<32>::GetMaximumValue());
	CBigInteger<16> iv = CBigInteger<16>(&randomNumber.GetData()[0]);
	std::vector<unsigned char> salt(randomNumber.GetData().cbegin() + 16, randomNumber.GetData().cbegin() + 24);

	CBigInteger<32> passwordHash = CalculatePasswordHash(password, salt);

	const std::vector<unsigned char>& walletSeedBytes = walletSeed.GetData();

	const CBigInteger<32> hash256 = Crypto::HMAC_SHA256(walletSeedBytes, passwordHash.GetData());
	const std::vector<unsigned char>& hash256Bytes = hash256.GetData();

	std::vector<unsigned char> seedPlusHash;
	seedPlusHash.insert(seedPlusHash.begin(), walletSeedBytes.cbegin(), walletSeedBytes.cend());
	seedPlusHash.insert(seedPlusHash.end(), hash256Bytes.cbegin(), hash256Bytes.cend());

	std::vector<unsigned char> encrypted = Crypto::AES256_Encrypt(seedPlusHash, passwordHash, iv);

	return std::make_tuple<Salt, IV, EncryptedBytes>(std::move(salt), std::move(iv), std::move(encrypted));
}

CBigInteger<32> SeedEncrypter::CalculatePasswordHash(const std::string& password, const std::vector<unsigned char>& salt) const
{
	const std::vector<unsigned char> passwordBytes(&password[0], &password[0] + password.length());
	const CBigInteger<64> scryptHash = Crypto::Scrypt(passwordBytes, salt);
	return Crypto::Blake2b(scryptHash.GetData());
}