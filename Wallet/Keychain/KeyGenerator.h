#pragma once

#include "KeyChainType.h"

#include <Wallet/PrivateExtKey.h>
#include <Wallet/PublicExtKey.h>

#include <Common/Secure.h>
#include <Crypto/SecretKey.h>
#include <Config/Config.h>

//
// For specifications on generating the master key for HD wallets, see https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#Master_key_generation.
//
class KeyGenerator
{
public:
	KeyGenerator(const Config& config);

	//
	// Generates a master private key for an HD wallet from a 32 byte seed.
	//
	// \param[in] seed - The 32 byte seed.
	//
	// \return The generated master CExtendedPrivateKey.
	//
	PrivateExtKey GenerateMasterKey(const SecureVector& seed, const EKeyChainType& keyChainType) const;

	//
	// Generates an extended child private key from a parent extended key (for an HD wallet).
	// Referred to as CKDpriv in BIP0032.
	//
	// \param[in] parentExtendedKey - The HD wallet's parent private key plus chain code.
	// \param[in] childKeyIndex - if >= 2^31, generate a 'hardened' child key (See BIP0032).
	//
	// \return The generated child CExtendedPrivateKey.
	//
	std::unique_ptr<PrivateExtKey> GenerateChildPrivateKey(const PrivateExtKey& parentExtendedKey, const uint32_t childKeyIndex) const;

	//
	// Generates an extended child public key from an extended parent public key (for an HD wallet).
	// Referred to as CKDpub in BIP0032.
	//
	// \param[in] parentExtendedPublicKey - The HD wallet's parent public key plus chain code.
	// \param[in] childKeyIndex - Must be < 2^31. 'Hardened' child keys need parent private keys to generate.
	//
	// \return The generated child CExtendedPublicKey.
	//
	PublicExtKey GenerateChildPublicKey(const PublicExtKey& parentExtendedPublicKey, const uint32_t childKeyIndex) const;

private:
	std::vector<unsigned char> GetSeed(const EKeyChainType& keyChainType) const;

	const Config& m_config;
};