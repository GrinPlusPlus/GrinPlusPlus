#pragma once

#include <Crypto/Models/SecretKey.h>
#include <Crypto/Models/PublicKey.h>
#include <secp256k1-zkp/secp256k1.h>
#include <shared_mutex>

class PublicKeys
{
public:
	static PublicKeys& GetInstance();
	PublicKeys()
	{
		m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
	}

	~PublicKeys()
	{
		secp256k1_context_destroy(m_pContext);
	}

	PublicKey CalculatePublicKey(const SecretKey& privateKey) const;
	PublicKey PublicKeySum(const std::vector<PublicKey>& publicKeys) const;

private:
	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
};