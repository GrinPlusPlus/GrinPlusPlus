#include "KeyGenerator.h"
#include "KeyDefs.h"

#include <Crypto/Crypto.h>
#include <Crypto/Hasher.h>
#include <Common/Util/BitUtil.h>
#include <Core/Serialization/Serializer.h>
#include <Wallet/PublicExtKey.h>

static const std::vector<uint8_t> KEY({ 'I','a','m','V','o', 'l', 'd', 'e', 'm', 'o', 'r', 't' });

PrivateExtKey KeyGenerator::GenerateMasterKey(const SecureVector& seed) const
{
	const CBigInteger<64> hash = Hasher::HMAC_SHA512(KEY, seed.data(), seed.size());

	CBigInteger<32> masterSecretKey(hash.data());

	if (masterSecretKey == KeyDefs::BIG_INT_ZERO || masterSecretKey >= KeyDefs::SECP256K1_N) {
		throw std::out_of_range("The seed resulted in an invalid private key."); // Less than 2^127 chance.
	}

	CBigInteger<32> masterChainCode(&hash[32]);

	return PrivateExtKey::Create(m_config.GetPrivateKeyVersion(), 0, 0, 0, std::move(masterChainCode), std::move(masterSecretKey));
}

PrivateExtKey KeyGenerator::GenerateChildPrivateKey(const PrivateExtKey& parentExtendedKey, const uint32_t childKeyIndex) const
{
	SecretKey parentChainCode = parentExtendedKey.GetChainCode();
	PublicKey parentCompressedKey = Crypto::CalculatePublicKey(parentExtendedKey.GetPrivateKey());

	PublicExtKey publicKey = PublicExtKey(
		parentExtendedKey.GetNetwork(),
		parentExtendedKey.GetDepth(),
		parentExtendedKey.GetParentFingerprint(),
		parentExtendedKey.GetChildNumber(),
		std::move(parentChainCode),
		std::move(parentCompressedKey)
	);

	CBigInteger<20> parentIdentifier = Hasher::RipeMD160(Hasher::SHA256(publicKey.vec()).GetData());
	const uint32_t parentFingerprint = BitUtil::ConvertToU32(parentIdentifier[0], parentIdentifier[1], parentIdentifier[2], parentIdentifier[3]);

	Serializer serializer(37); // Reserve 37 bytes: 1 byte for 0x00 padding (hardened) or 0x02/0x03 point parity (normal), 32 bytes for private key (hardened) or public key X coord, 4 bytes for index.

	if (childKeyIndex >= KeyDefs::MINIMUM_HARDENED_INDEX)
	{
		// Generate a hardened child key
		serializer.Append<uint8_t>(0x00);
		serializer.AppendBigInteger<32>(parentExtendedKey.GetPrivateKey().GetBytes());
	}
	else
	{
		// Generate a normal child key
		serializer.AppendBigInteger<33>(publicKey.GetPublicKey().GetCompressedBytes());
	}

	serializer.Append<uint32_t>(childKeyIndex);

	const std::vector<uint8_t>& serialized_bytes = serializer.GetBytes();
	const CBigInteger<64> hmacSha512 = Hasher::HMAC_SHA512(parentExtendedKey.GetChainCode().GetVec(), serialized_bytes.data(), serialized_bytes.size());
	const std::vector<unsigned char>& hmacSha512Vector = hmacSha512.GetData();

	SecretKey left(std::vector<uint8_t>{ hmacSha512Vector.cbegin(), hmacSha512Vector.cbegin() + 32 });

	SecretKey childPrivateKey = Crypto::AddPrivateKeys(left, parentExtendedKey.GetPrivateKey());

	CBigInteger<32> childChainCode(std::vector<uint8_t>{ hmacSha512Vector.cbegin() + 32, hmacSha512Vector.cbegin() + 64 });

	return PrivateExtKey::Create(
		m_config.GetPrivateKeyVersion(),
		parentExtendedKey.GetDepth() + 1,
		parentFingerprint, childKeyIndex,
		std::move(childChainCode),
		std::move(childPrivateKey)
	);
}