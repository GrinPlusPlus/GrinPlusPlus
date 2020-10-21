#include <Crypto/Crypto.h>
#include <Core/Traits/Lockable.h>
#include <cassert>

#include "Context.h"

// Secp256k1
#include "AggSig.h"
#include "Bulletproofs.h"
#include "Pedersen.h"
#include "PublicKeys.h"

#ifdef _WIN32
#pragma comment(lib, "crypt32")
#endif

Locked<secp256k1::Context> SECP256K1_CONTEXT(std::make_shared<secp256k1::Context>());

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
	SecretKey result(secretKey1.GetVec());

	const int tweak_result = secp256k1_ec_privkey_tweak_add(SECP256K1_CONTEXT.Read()->Get(), result.data(), secretKey2.data());
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

CompactSignature Crypto::CalculatePartialSignature(const SecretKey& secretKey, const SecretKey& secretNonce, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message)
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