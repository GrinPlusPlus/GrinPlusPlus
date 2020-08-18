#include <Crypto/Crypto.h>
#include <Crypto/CryptoException.h>
#include <Common/Logger.h>
#include <sodium/crypto_generichash_blake2b.h>
#include <cassert>

#include "ThirdParty/siphash.h"

#include <bitcoin/sha256.h>
#include <bitcoin/ripemd160.h>
#include <bitcoin/hmac_sha256.h>
#include <bitcoin/hmac_sha512.h>
#include <bitcoin/aes.h>
#include <scrypt/crypto_scrypt.h>
#include <sha.h>

// Secp256k1
#include "AggSig.h"
#include "Bulletproofs.h"
#include "Pedersen.h"
#include "PublicKeys.h"

#ifdef _WIN32
#pragma comment(lib, "crypt32")
#endif

CBigInteger<32> Crypto::Blake2b(const std::vector<unsigned char>& input)
{
	CBigInteger<32> output;

	int result = crypto_generichash_blake2b(output.data(), output.size(), input.data(), input.size(), nullptr, 0);
	assert(result == 0);

	return output;
}

CBigInteger<32> Crypto::Blake2b(const std::vector<unsigned char>& key, const std::vector<unsigned char>& input)
{
	CBigInteger<32> output;

	int result = crypto_generichash_blake2b(output.data(), output.size(), input.data(), input.size(), key.data(), key.size());
	assert(result == 0);

	return output;
}

CBigInteger<32> Crypto::SHA256(const std::vector<unsigned char> & input)
{
	std::vector<unsigned char> sha256(32, 0);

	CSHA256().Write(input.data(), input.size()).Finalize(sha256.data());

	return CBigInteger<32>(sha256);
}

CBigInteger<64> Crypto::SHA512(const std::vector<unsigned char> & input)
{
	std::vector<unsigned char> sha512(64, 0);

	CSHA512().Write(input.data(), input.size()).Finalize(sha512.data());

	return CBigInteger<64>(sha512);
}

CBigInteger<20> Crypto::RipeMD160(const std::vector<unsigned char>& input)
{
	std::vector<unsigned char> ripemd(32, 0);

	CRIPEMD160().Write(input.data(), input.size()).Finalize(ripemd.data());

	return CBigInteger<20>(ripemd);
}

CBigInteger<32> Crypto::HMAC_SHA256(const std::vector<unsigned char>& key, const std::vector<unsigned char>& data)
{
	std::vector<unsigned char> result(32);

	CHMAC_SHA256(key.data(), key.size()).Write(data.data(), data.size()).Finalize(result.data());

	return CBigInteger<32>(result.data());
}

CBigInteger<64> Crypto::HMAC_SHA512(const std::vector<unsigned char>& key, const uint8_t* data, const size_t len)
{
	CBigInteger<64> result;

	CHMAC_SHA512(key.data(), key.size()).Write(data, len).Finalize(result.data());

	return result;
}

Commitment Crypto::CommitTransparent(const uint64_t value)
{
	const BlindingFactor blindingFactor(CBigInteger<32>::ValueOf(0));

	return Pedersen::GetInstance().PedersenCommit(value, blindingFactor);
}

Commitment Crypto::CommitBlinded(const uint64_t value, const BlindingFactor& blindingFactor)
{
	return Pedersen::GetInstance().PedersenCommit(value, blindingFactor);
}

Commitment Crypto::AddCommitments(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative)
{
	const Commitment zeroCommitment(CBigInteger<33>::ValueOf(0));

	std::vector<Commitment> sanitizedPositive;
	std::copy_if(
		positive.cbegin(),
		positive.cend(),
		std::back_inserter(sanitizedPositive),
		[&zeroCommitment](const Commitment& positiveCommitment) { return positiveCommitment != zeroCommitment; }
	);

	std::vector<Commitment> sanitizedNegative;
	std::copy_if(
		negative.cbegin(),
		negative.cend(),
		std::back_inserter(sanitizedNegative),
		[&zeroCommitment](const Commitment& negativeCommitment) { return negativeCommitment != zeroCommitment; }
	);

	return Pedersen::GetInstance().PedersenCommitSum(sanitizedPositive, sanitizedNegative);
}

BlindingFactor Crypto::AddBlindingFactors(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative)
{
	BlindingFactor zeroBlindingFactor(ZERO_HASH);

	std::vector<BlindingFactor> sanitizedPositive;
	std::copy_if(
		positive.cbegin(),
		positive.cend(),
		std::back_inserter(sanitizedPositive),
		[&zeroBlindingFactor](const BlindingFactor& positiveBlind) { return positiveBlind != zeroBlindingFactor; }
	);

	std::vector<BlindingFactor> sanitizedNegative;
	std::copy_if(
		negative.cbegin(),
		negative.cend(),
		std::back_inserter(sanitizedNegative),
		[&zeroBlindingFactor](const BlindingFactor& negativeBlind) { return negativeBlind != zeroBlindingFactor; }
	);

	if (sanitizedPositive.empty() && sanitizedNegative.empty())
	{
		return zeroBlindingFactor;
	}

	return Pedersen::GetInstance().PedersenBlindSum(sanitizedPositive, sanitizedNegative);
}

SecretKey Crypto::BlindSwitch(const SecretKey& secretKey, const uint64_t amount)
{
	return Pedersen::GetInstance().BlindSwitch(secretKey, amount);
}

// Temporary hack since creating a context too often crashes on mac & linux
struct SecpContext
{
	SecpContext()
	{
		pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
	}

	~SecpContext()
	{
		secp256k1_context_destroy(pContext);
	}

	secp256k1_context* pContext;
};

static SecpContext secp_context;

SecretKey Crypto::AddPrivateKeys(const SecretKey& secretKey1, const SecretKey& secretKey2)
{
	SecretKey result(secretKey1.GetVec());
	const int tweak_result = secp256k1_ec_privkey_tweak_add(secp_context.pContext, result.data(), secretKey2.data());

	if (tweak_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_privkey_tweak_add failed with error {}", tweak_result);
	}

	return result;
}

RangeProof Crypto::GenerateRangeProof(const uint64_t amount, const SecretKey& key, const SecretKey& privateNonce, const SecretKey& rewindNonce, const ProofMessage& proofMessage)
{
	return Bulletproofs::GetInstance().GenerateRangeProof(amount, key, privateNonce, rewindNonce, proofMessage);
}

std::unique_ptr<RewoundProof> Crypto::RewindRangeProof(const Commitment& commitment, const RangeProof& rangeProof, const SecretKey& nonce)
{
	return Bulletproofs::GetInstance().RewindProof(commitment, rangeProof, nonce);
}

bool Crypto::VerifyRangeProofs(const std::vector<std::pair<Commitment, RangeProof>>& rangeProofs)
{
	return Bulletproofs::GetInstance().VerifyBulletproofs(rangeProofs);
}

uint64_t Crypto::SipHash24(const uint64_t k0, const uint64_t k1, const std::vector<unsigned char>& data)
{
	const std::vector<uint64_t> key = { k0, k1 };

	return siphash24(&key[0], &data[0], data.size());
}

std::vector<unsigned char> Crypto::AES256_Encrypt(const SecureVector& input, const SecretKey& key, const CBigInteger<16>& iv)
{   
	std::vector<unsigned char> ciphertext;

	// max ciphertext len for a n bytes of plaintext is n + AES_BLOCKSIZE bytes
	ciphertext.resize(input.size() + AES_BLOCKSIZE);

	AES256CBCEncrypt enc(key.data(), iv.data(), true);
	const size_t nLen = enc.Encrypt(&input[0], (int)input.size(), ciphertext.data());
	if (nLen < input.size())
	{
		throw CryptoException();
	}

	ciphertext.resize(nLen);

	return ciphertext ;
}

SecureVector Crypto::AES256_Decrypt(const std::vector<unsigned char>& ciphertext, const SecretKey& key, const CBigInteger<16>& iv)
{    
	SecureVector plaintext;

	// plaintext will always be equal to or lesser than length of ciphertext
	size_t nLen = ciphertext.size();

	plaintext.resize(nLen);

	AES256CBCDecrypt dec(key.data(), iv.data(), true);
	nLen = dec.Decrypt(ciphertext.data(), (int)ciphertext.size(), plaintext.data());
	if (nLen == 0)
	{
		throw CryptoException();
	}

	plaintext.resize(nLen);

	return plaintext;
}

SecretKey Crypto::PBKDF(const SecureString& password, const std::vector<unsigned char>& salt, const ScryptParameters& parameters)
{
	SecureVector buffer(64);
	if (crypto_scrypt((const unsigned char*)password.data(), password.size(), salt.data(), salt.size(), parameters.N, parameters.r, parameters.p, buffer.data(), buffer.size()) == 0)
	{
		CBigInteger<32> output;

		int result = crypto_generichash_blake2b(output.data(), output.size(), buffer.data(), buffer.size(), nullptr, 0);
		assert(result == 0);

		return SecretKey(std::move(output));
	}

	throw CryptoException();
}

PublicKey Crypto::CalculatePublicKey(const SecretKey& privateKey)
{
	return PublicKeys::GetInstance().CalculatePublicKey(privateKey);
}

PublicKey Crypto::AddPublicKeys(const std::vector<PublicKey>& publicKeys)
{
	return PublicKeys::GetInstance().PublicKeySum(publicKeys);
}

Commitment Crypto::ToCommitment(const PublicKey& publicKey)
{
	return Pedersen::GetInstance().ToCommitment(publicKey);
}

std::unique_ptr<CompactSignature> Crypto::CalculatePartialSignature(const SecretKey& secretKey, const SecretKey& secretNonce, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message)
{
	return AggSig::GetInstance().CalculatePartialSignature(secretKey, secretNonce, sumPubKeys, sumPubNonces, message);
}

std::unique_ptr<Signature> Crypto::AggregateSignatures(const std::vector<CompactSignature>& signatures, const PublicKey& sumPubNonces)
{
	return AggSig::GetInstance().AggregateSignatures(signatures, sumPubNonces);
}

std::unique_ptr<Signature> Crypto::BuildCoinbaseSignature(const SecretKey& secretKey, const Commitment& commitment, const Hash& message)
{
	return AggSig::GetInstance().BuildSignature(secretKey, commitment, message);
}

bool Crypto::VerifyPartialSignature(const CompactSignature& partialSignature, const PublicKey& publicKey, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message)
{
	return AggSig::GetInstance().VerifyPartialSignature(partialSignature, publicKey, sumPubKeys, sumPubNonces, message);
}

bool Crypto::VerifyAggregateSignature(const Signature& aggregateSignature, const PublicKey& sumPubKeys, const Hash& message)
{
	return AggSig::GetInstance().VerifyAggregateSignature(aggregateSignature, sumPubKeys, message);
}

Signature Crypto::ParseCompactSignature(const CompactSignature& signature)
{
	auto sigs = AggSig::GetInstance().ParseCompactSignatures({ signature });

	return Signature(sigs.front().data);
}

CompactSignature Crypto::ToCompact(const Signature& signature)
{
	return AggSig::GetInstance().ToCompact(signature);
}

bool Crypto::VerifyKernelSignatures(const std::vector<const Signature*>& signatures, const std::vector<const Commitment*>& publicKeys, const std::vector<const Hash*>& messages)
{
	return AggSig::GetInstance().VerifyAggregateSignatures(signatures, publicKeys, messages);
}

SecretKey Crypto::GenerateSecureNonce()
{
	return AggSig::GetInstance().GenerateSecureNonce();
}

SecretKey Crypto::HKDF(const std::optional<std::vector<uint8_t>>& saltOpt,
	const std::string& label,
	const std::vector<uint8_t>& inputKeyingMaterial)
{
	SecretKey output;

	//std::vector<uint8_t> info(label.cbegin(), label.cend());
	const int result = hkdf(
		SHAversion::SHA256,
		saltOpt.has_value() ? saltOpt.value().data() : nullptr,
		saltOpt.has_value() ? (int)saltOpt.value().size() : 0,
		inputKeyingMaterial.data(),
		(int)inputKeyingMaterial.size(),
		(const unsigned char*)label.data(),
		(int)label.size(),
		output.data(),
		(int)output.size()
	);
	if (result != shaSuccess) {
		throw CryptoException();
	}

	return output;
}