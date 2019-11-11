#include "PublicKeys.h"

#include "secp256k1-zkp/include/secp256k1.h"
#include <Crypto/CryptoException.h>

PublicKeys& PublicKeys::GetInstance()
{
	static PublicKeys instance;
	return instance;
}

PublicKeys::PublicKeys()
{
	m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
}

PublicKeys::~PublicKeys()
{
	secp256k1_context_destroy(m_pContext);
}

PublicKey PublicKeys::CalculatePublicKey(const SecretKey& privateKey) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	const int verifyResult = secp256k1_ec_seckey_verify(m_pContext, privateKey.data());
	if (verifyResult == 1)
	{
		secp256k1_pubkey pubkey;
		const int createResult = secp256k1_ec_pubkey_create(m_pContext, &pubkey, privateKey.data());
		if (createResult == 1)
		{
			size_t length = 33;
			std::vector<unsigned char> serializedPublicKey(length);
			const int serializeResult = secp256k1_ec_pubkey_serialize(m_pContext, serializedPublicKey.data(), &length, &pubkey, SECP256K1_EC_COMPRESSED);
			if (serializeResult == 1)
			{
				return PublicKey(std::move(serializedPublicKey));
			}
		}
	}

	throw CryptoException("Failed to calculate public key");
}

PublicKey PublicKeys::PublicKeySum(const std::vector<PublicKey>& publicKeys) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pubkey*> parsedPubKeys;
	for (const PublicKey& publicKey : publicKeys)
	{
		secp256k1_pubkey* pPublicKey = new secp256k1_pubkey();
		int pubKeyParsed = secp256k1_ec_pubkey_parse(m_pContext, pPublicKey, publicKey.data(), publicKey.size());
		if (pubKeyParsed == 1)
		{
			parsedPubKeys.push_back(pPublicKey);
		}
		else
		{
			delete pPublicKey;
			for (secp256k1_pubkey* pParsedPubKey : parsedPubKeys)
			{
				delete pParsedPubKey;
			}

			throw CryptoException("secp256k1_ec_pubkey_parse failed with error: " + std::to_string(pubKeyParsed));
		}
	}

	secp256k1_pubkey publicKey;
	const int pubKeysCombined = secp256k1_ec_pubkey_combine(m_pContext, &publicKey, parsedPubKeys.data(), parsedPubKeys.size());

	for (secp256k1_pubkey* pParsedPubKey : parsedPubKeys)
	{
		delete pParsedPubKey;
	}

	if (pubKeysCombined == 1)
	{
		size_t length = 33;
		std::vector<unsigned char> serializedPublicKey(length);
		const int serializeResult = secp256k1_ec_pubkey_serialize(m_pContext, serializedPublicKey.data(), &length, &publicKey, SECP256K1_EC_COMPRESSED);
		if (serializeResult == 1)
		{
			return PublicKey(CBigInteger<33>(std::move(serializedPublicKey)));
		}
	}

	throw CryptoException("Failed to combine public keys.");
}