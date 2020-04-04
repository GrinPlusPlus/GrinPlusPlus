#include <Crypto/Crypto.h>
#include <Crypto/CryptoException.h>
#include <Infrastructure/Logger.h>

#include "ThirdParty/Blake2.h"
#include "ThirdParty/sha256.h"
#include "ThirdParty/ripemd160.h"
#include "ThirdParty/hmac_sha256.h"
#include "ThirdParty/hmac_sha512.h"
#include "ThirdParty/aes.h"
#include "ThirdParty/siphash.h"
#include <scrypt/crypto_scrypt.h>

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
	std::vector<unsigned char> tmp(32, 0);

	blake2b(&tmp[0], 32, &input[0], input.size(), nullptr, 0);

	return CBigInteger<32>(&tmp[0]);
}

CBigInteger<32> Crypto::Blake2b(const std::vector<unsigned char>& key, const std::vector<unsigned char>& input)
{
	std::vector<unsigned char> tmp(32, 0);

	blake2b(&tmp[0], 32, input.data(), input.size(), key.data(), key.size());

	return CBigInteger<32>(&tmp[0]);
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

CBigInteger<64> Crypto::HMAC_SHA512(const std::vector<unsigned char>& key, const std::vector<unsigned char>& data)
{
	std::vector<unsigned char> result(64);

	CHMAC_SHA512(key.data(), key.size()).Write(data.data(), data.size()).Finalize(result.data());

	return CBigInteger<64>(std::move(result));
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

SecretKey Crypto::AddPrivateKeys(const SecretKey& secretKey1, const SecretKey& secretKey2)
{
	secp256k1_context* pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

	CBigInteger<32> result(secretKey1.GetVec());
	if (secp256k1_ec_privkey_tweak_add(pContext, (unsigned char*)result.data(), secretKey2.data()) == 1)
	{
		return SecretKey(std::move(result));
	}

	throw CryptoException("secp256k1_ec_privkey_tweak_add failed");
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
		std::vector<unsigned char> tmp(32, 0);

		blake2b(&tmp[0], 32, &buffer[0], buffer.size(), nullptr, 0);

		return SecretKey(CBigInteger<32>(&tmp[0]));
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

std::unique_ptr<CompactSignature> Crypto::SignMessage(const SecretKey& secretKey, const PublicKey& publicKey, const std::string& message)
{
	const Hash messageHash = Crypto::Blake2b(std::vector<unsigned char>(message.cbegin(), message.cend()));
	return AggSig::GetInstance().SignMessage(secretKey, publicKey, messageHash);
}

bool Crypto::VerifyMessageSignature(const CompactSignature& signature, const PublicKey& publicKey, const std::string& message)
{
	const Hash messageHash = Crypto::Blake2b(std::vector<unsigned char>(message.cbegin(), message.cend()));
	return AggSig::GetInstance().VerifyMessageSignature(signature, publicKey, messageHash);
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

bool Crypto::VerifyAggregateSignature(const Signature& aggregateSignature, const PublicKey sumPubKeys, const Hash& message)
{
	return AggSig::GetInstance().VerifyAggregateSignature(aggregateSignature, sumPubKeys, message);
}

bool Crypto::VerifyKernelSignatures(const std::vector<const Signature*>& signatures, const std::vector<const Commitment*>& publicKeys, const std::vector<const Hash*>& messages)
{
	return AggSig::GetInstance().VerifyAggregateSignatures(signatures, publicKeys, messages);
}

SecretKey Crypto::GenerateSecureNonce()
{
	return AggSig::GetInstance().GenerateSecureNonce();
}