#pragma once

#include <Crypto/SecretKey.h>
#include <Crypto/PublicKey.h>

// Forward Declarations
typedef struct secp256k1_context_struct secp256k1_context;

class ECDH
{
public:
	static ECDH& GetInstance();

	std::unique_ptr<SecretKey> CalculateSharedSecret(const SecretKey& privateKey, const PublicKey& publicKey) const;

private:
	ECDH();
	~ECDH();

	secp256k1_context* m_pContext;
};