#pragma once

#include <Wallet/PrivateExtKey.h>

#include <Common/Secure.h>
#include <Core/Config.h>

//
// For specifications on generating the master key for HD wallets, see https://github.com/bitcoin/bips/blob/master/bip-0032.mediawiki#Master_key_generation.
//
class KeyGenerator
{
public:
	KeyGenerator(const Config& config) : m_config(config) { }

	//
	// Generates a master private key for an HD wallet from a 32 byte seed.
	//
	// \param[in] seed - The 32 byte seed.
	//
	// \return The generated master CExtendedPrivateKey.
	//
	PrivateExtKey GenerateMasterKey(const SecureVector& seed) const;

	//
	// Generates an extended child private key from a parent extended key (for an HD wallet).
	// Referred to as CKDpriv in BIP0032.
	//
	// \param[in] parentExtendedKey - The HD wallet's parent private key plus chain code.
	// \param[in] childKeyIndex - if >= 2^31, generate a 'hardened' child key (See BIP0032).
	//
	// \return The generated child CExtendedPrivateKey.
	//
	PrivateExtKey GenerateChildPrivateKey(const PrivateExtKey& parentExtendedKey, const uint32_t childKeyIndex) const;

private:
	const Config& m_config;
};