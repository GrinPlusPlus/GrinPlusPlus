#include "WalletEncryptionUtil.h"

#include <Crypto/Crypto.h>
#include <Crypto/RandomNumberGenerator.h>

static const uint8_t ENCRYPTION_FORMAT = 0;

std::vector<unsigned char> WalletEncryptionUtil::Encrypt(
	const SecureVector& masterSeed,
	const std::string& dataType,
	const SecureVector& bytes)
{
	const CBigInteger<32> randomNumber = RandomNumberGenerator::GenerateRandom32();
	const CBigInteger<16> iv = CBigInteger<16>(&randomNumber[0]);
	const SecretKey key = WalletEncryptionUtil::CreateSecureKey(masterSeed, dataType);

	const std::vector<unsigned char> encryptedBytes =
		Crypto::AES256_Encrypt(bytes, key, iv);

	Serializer serializer;
	serializer.Append<uint8_t>(ENCRYPTION_FORMAT);
	serializer.AppendBigInteger(iv);
	serializer.AppendByteVector(encryptedBytes);
	return serializer.GetBytes();
}

SecureVector WalletEncryptionUtil::Decrypt(
	const SecureVector& masterSeed,
	const std::string& dataType,
	const std::vector<unsigned char>& encrypted)
{
	ByteBuffer byteBuffer(encrypted);

	const uint8_t formatVersion = byteBuffer.ReadU8();
	if (formatVersion != ENCRYPTION_FORMAT)
	{
		throw DESERIALIZATION_EXCEPTION_F(
			"Expected format {}, but was {}",
			ENCRYPTION_FORMAT,
			formatVersion
		);
	}

	const CBigInteger<16> iv = byteBuffer.ReadBigInteger<16>();
	const std::vector<unsigned char> encryptedBytes =
		byteBuffer.ReadVector(byteBuffer.GetRemainingSize());
	const SecretKey key = WalletEncryptionUtil::CreateSecureKey(masterSeed, dataType);

	return Crypto::AES256_Decrypt(encryptedBytes, key, iv);
}

SecretKey WalletEncryptionUtil::CreateSecureKey(
	const SecureVector& masterSeed,
	const std::string& dataType)
{
	SecureVector seedWithNonce(masterSeed.data(), masterSeed.data() + masterSeed.size());

	Serializer nonceSerializer;
	nonceSerializer.AppendVarStr(dataType);
	seedWithNonce.insert(
		seedWithNonce.end(),
		nonceSerializer.GetBytes().begin(),
		nonceSerializer.GetBytes().end()
	);

	return Crypto::Blake2b((const std::vector<unsigned char>&)seedWithNonce);
}