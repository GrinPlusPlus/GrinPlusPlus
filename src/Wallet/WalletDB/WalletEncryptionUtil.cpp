#include "WalletEncryptionUtil.h"

#include <Crypto/AES256.h>
#include <Crypto/Hasher.h>
#include <Crypto/CSPRNG.h>

static const uint8_t ENCRYPTION_FORMAT = 0;

std::vector<uint8_t> WalletEncryptionUtil::Encrypt(
	const SecureVector& masterSeed,
	const std::string& dataType,
	const SecureVector& bytes)
{
	const SecureVector randomNumber = CSPRNG::GenerateRandomBytes(16);
	const CBigInteger<16> iv = CBigInteger<16>(randomNumber.data());
	const SecretKey key = WalletEncryptionUtil::CreateSecureKey(masterSeed, dataType);

	const std::vector<uint8_t> encryptedBytes = AES256::Encrypt(bytes, key, iv);

	Serializer serializer;
	serializer.Append<uint8_t>(ENCRYPTION_FORMAT);
	serializer.AppendBigInteger(iv);
	serializer.AppendByteVector(encryptedBytes);
	return serializer.GetBytes();
}

SecureVector WalletEncryptionUtil::Decrypt(
	const SecureVector& masterSeed,
	const std::string& dataType,
	const std::vector<uint8_t>& encrypted)
{
	ByteBuffer byteBuffer(encrypted);

	const uint8_t formatVersion = byteBuffer.ReadU8();
	if (formatVersion != ENCRYPTION_FORMAT) {
		throw DESERIALIZATION_EXCEPTION_F(
			"Expected format {}, but was {}",
			ENCRYPTION_FORMAT,
			formatVersion
		);
	}

	const CBigInteger<16> iv = byteBuffer.ReadBigInteger<16>();
	const std::vector<uint8_t> encryptedBytes =
		byteBuffer.ReadVector(byteBuffer.GetRemainingSize());
	const SecretKey key = WalletEncryptionUtil::CreateSecureKey(masterSeed, dataType);

	return AES256::Decrypt(encryptedBytes, key, iv);
}

SecretKey WalletEncryptionUtil::CreateSecureKey(
	const SecureVector& masterSeed,
	const std::string& dataType)
{
	SecureVector seedWithNonce(masterSeed.cbegin(), masterSeed.cend());

	Serializer nonceSerializer;
	nonceSerializer.AppendVarStr(dataType);
	seedWithNonce.insert(
		seedWithNonce.end(),
		nonceSerializer.GetBytes().begin(),
		nonceSerializer.GetBytes().end()
	);

	return Hasher::Blake2b(seedWithNonce.data(), seedWithNonce.size());
}