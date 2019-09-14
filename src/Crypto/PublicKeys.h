#pragma once

#include <Crypto/SecretKey.h>
#include <Crypto/PublicKey.h>
#include <shared_mutex>

// Forward Declarations
typedef struct secp256k1_context_struct secp256k1_context;

class PublicKeys
{
public:
	static PublicKeys& GetInstance();

	std::unique_ptr<PublicKey> CalculatePublicKey(const SecretKey& privateKey) const;
	std::unique_ptr<PublicKey> PublicKeySum(const std::vector<PublicKey>& publicKeys) const;

private:
	PublicKeys();
	~PublicKeys();

	mutable std::shared_mutex m_mutex;
	secp256k1_context* m_pContext;
};