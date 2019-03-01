#include "KeyGenerator.h"
#include "KeyDefs.h"
#include "PublicKeyCalculator.h"

#include <Crypto/Crypto.h>
#include <Common/Util/BitUtil.h>
#include <Common/Util/VectorUtil.h>
#include <Core/Serialization/Serializer.h>

KeyGenerator::KeyGenerator(const Config& config)
	: m_config(config)
{

}

PrivateExtKey KeyGenerator::GenerateMasterKey(const CBigInteger<32>& seed) const
{
	unsigned char key[] = { 'I','a','m','V','o', 'l', 'd', 'e', 'm', 'o', 'r', 't' };
	std::vector<unsigned char> vchKey = VectorUtil::MakeVector<unsigned char, 12>(key);
	const CBigInteger<64> hash = Crypto::HMAC_SHA512(vchKey, seed.GetData());
	const std::vector<unsigned char>& vchHash = hash.GetData();

	CBigInteger<32> masterSecretKey(&vchHash[0]);

	if (masterSecretKey == KeyDefs::BIG_INT_ZERO || masterSecretKey >= KeyDefs::SECP256K1_N)
	{
		throw std::out_of_range("The seed resulted in an invalid private key."); // Less than 2^127 chance.
	}

	CBigInteger<32> masterChainCode(&vchHash[32]);

	return PrivateExtKey::Create(m_config.GetWalletConfig().GetPrivateKeyVersion(), 0, 0, 0, std::move(masterChainCode), std::move(masterSecretKey));
}

std::unique_ptr<PrivateExtKey> KeyGenerator::GenerateChildPrivateKey(const PrivateExtKey& parentExtendedKey, const uint32_t childKeyIndex) const
{
	std::unique_ptr<PublicExtKey> pPublicKey = PublicKeyCalculator().DeterminePublicKey(parentExtendedKey);
	if (pPublicKey == nullptr)
	{
		return std::unique_ptr<PrivateExtKey>(nullptr);
	}

	CBigInteger<20> parentIdentifier = Crypto::RipeMD160(Crypto::SHA256(pPublicKey->GetPublicKey().GetData()).GetData());
	const uint32_t parentFingerprint = BitUtil::ConvertToU32(parentIdentifier[0], parentIdentifier[1], parentIdentifier[2], parentIdentifier[3]);

	Serializer serializer(37); // Reserve 37 bytes: 1 byte for 0x00 padding (hardened) or 0x02/0x03 point parity (normal), 32 bytes for private key (hardened) or public key X coord, 4 bytes for index.

	if (childKeyIndex >= KeyDefs::MINIMUM_HARDENED_INDEX)
	{
		// Generate a hardened child key
		serializer.Append<uint8_t>(0x00);
		serializer.AppendBigInteger<32>(parentExtendedKey.GetPrivateKey());
	}
	else
	{
		// Generate a normal child key
		serializer.AppendBigInteger<33>(pPublicKey->GetPublicKey());
	}

	serializer.Append<uint32_t>(EndianHelper::GetBigEndian32(childKeyIndex));

	const CBigInteger<64> hmacSha512 = Crypto::HMAC_SHA512(parentExtendedKey.GetChainCode().GetData(), serializer.GetBytes());
	const std::vector<unsigned char>& hmacSha512Vector = hmacSha512.GetData();

	std::vector<unsigned char> vchLeft;
	vchLeft.insert(vchLeft.begin(), hmacSha512Vector.cbegin(), hmacSha512Vector.cbegin() + 32);
	const CBigInteger<32> left(std::move(vchLeft));

	if (left >= KeyDefs::SECP256K1_N)
	{
		throw std::out_of_range("The child key generated was invalid.");  // Less than 2^127 chance.
	}

	CBigInteger<32> childPrivateKey = left.addMod(parentExtendedKey.GetPrivateKey(), KeyDefs::SECP256K1_N);
	if (childPrivateKey == KeyDefs::BIG_INT_ZERO)
	{
		throw std::out_of_range("The child key generated was invalid."); // Less than 2^127 chance.
	}

	std::vector<unsigned char> vchRight;
	vchRight.insert(vchRight.begin(), hmacSha512Vector.cbegin() + 32, hmacSha512Vector.cbegin() + 64);
	CBigInteger<32> childChainCode(&vchRight[0]);

	return std::make_unique<PrivateExtKey>(PrivateExtKey::Create(m_config.GetWalletConfig().GetPrivateKeyVersion(), parentExtendedKey.GetDepth() + 1, parentFingerprint, childKeyIndex, std::move(childChainCode), std::move(childPrivateKey)));
}

PublicExtKey KeyGenerator::GenerateChildPublicKey(const PublicExtKey& parentExtendedPublicKey, const uint32_t childKeyIndex) const
{
	throw std::exception("Not implemented yet");
}