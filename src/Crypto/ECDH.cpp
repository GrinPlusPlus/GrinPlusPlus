#include "ECDH.h"

#include "secp256k1-zkp/include/secp256k1_ecdh.h"

ECDH& ECDH::GetInstance()
{
	static ECDH instance;
	return instance;
}

ECDH::ECDH()
{
	m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
}

ECDH::~ECDH()
{
	secp256k1_context_destroy(m_pContext);
}

std::unique_ptr<SecretKey> ECDH::CalculateSharedSecret(const SecretKey& privateKey, const PublicKey& publicKey) const
{
	const int verifyResult = secp256k1_ec_seckey_verify(m_pContext, privateKey.data());
	if (verifyResult == 1)
	{
		secp256k1_pubkey pubkey;
		const int parseResult = secp256k1_ec_pubkey_parse(m_pContext, &pubkey, publicKey.GetCompressedBytes().data(), publicKey.GetCompressedBytes().size());
		if (parseResult == 1)
		{
			SecureVector result;
			const int ecdhResult = secp256k1_ecdh(m_pContext, result.data(), &pubkey, privateKey.data());
			if (ecdhResult == 1)
			{
				return std::make_unique<SecretKey>(SecretKey(CBigInteger<32>((const std::vector<unsigned char>&)result)));
			}
		}
	}

	return std::unique_ptr<SecretKey>(nullptr);
}