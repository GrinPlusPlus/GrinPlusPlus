#pragma once

#include <Wallet/EncryptedSeed.h>
#include <Wallet/PrivateExtKey.h>
#include <Wallet/PublicExtKey.h>
#include <Wallet/KeyChainPath.h>
#include <Wallet/OutputData.h>

#include <Config/Config.h>
#include <Crypto/BlindingFactor.h>
#include <Common/Secure.h>
#include <vector>

class KeyChain
{
public:
	// TODO: Add FromMnemonic, FromRandom, ToMnemonic, and GetSeed methods
	static KeyChain FromSeed(const Config& config, const CBigInteger<32>& masterSeed);

	std::unique_ptr<BlindingFactor> DerivePrivateKey(const KeyChainPath& keyPath, const uint64_t amount) const;

	std::unique_ptr<RewoundProof> RewindRangeProof(const Commitment& commitment, const RangeProof& rangeProof) const;
	std::unique_ptr<RangeProof> GenerateRangeProof(const KeyChainPath& keyChainPath, const uint64_t amount, const Commitment& commitment, const BlindingFactor& blindingFactor) const;

private:
	KeyChain(const Config& config, PrivateExtKey&& masterKey, BlindingFactor&& bulletProofNonce);

	CBigInteger<32> CreateNonce(const Commitment& commitment) const;

	const Config& m_config;
	PrivateExtKey m_masterKey;
	BlindingFactor m_bulletProofNonce;
};