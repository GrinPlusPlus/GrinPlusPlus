#include "PublicKeys.h"

#include <Core/Exceptions/CryptoException.h>

static PublicKeys instance;

PublicKeys& PublicKeys::GetInstance()
{
	return instance;
}

PublicKey PublicKeys::CalculatePublicKey(const SecretKey& privateKey) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	const int verify_result = secp256k1_ec_seckey_verify(m_pContext, privateKey.data());
	if (verify_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_seckey_verify failed with error {}", verify_result);
	}

	secp256k1_pubkey pubkey;
	const int create_result = secp256k1_ec_pubkey_create(m_pContext, &pubkey, privateKey.data());
	if (create_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_pubkey_create failed with error {}", create_result);
	}

	PublicKey serialized_pubkey;
	size_t pubkey_len = serialized_pubkey.size();
	const int serialize_result = secp256k1_ec_pubkey_serialize(m_pContext, serialized_pubkey.data(), &pubkey_len, &pubkey, SECP256K1_EC_COMPRESSED);
	if (serialize_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_pubkey_serialize failed with error {}", serialize_result);
	}

	return serialized_pubkey;
}

PublicKey PublicKeys::PublicKeySum(const std::vector<PublicKey>& publicKeys) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pubkey> parsed_pubkeys;
	for (const PublicKey& publicKey : publicKeys)
	{
		secp256k1_pubkey parsed_pubkey;
		int parse_result = secp256k1_ec_pubkey_parse(m_pContext, &parsed_pubkey, publicKey.data(), publicKey.size());
		if (parse_result != 1) {
			throw CRYPTO_EXCEPTION_F("secp256k1_ec_pubkey_parse failed with error {}", parse_result);
		}
		
		parsed_pubkeys.push_back(parsed_pubkey);
	}

	std::vector<secp256k1_pubkey*> parsed_pubkey_ptrs;
	std::transform(
		parsed_pubkeys.begin(), parsed_pubkeys.end(),
		std::back_inserter(parsed_pubkey_ptrs),
		[] (secp256k1_pubkey& parsed_pubkey) { return &parsed_pubkey; }
	);

	secp256k1_pubkey publicKey;
	const int combined_result = secp256k1_ec_pubkey_combine(
		m_pContext,
		&publicKey,
		parsed_pubkey_ptrs.data(),
		parsed_pubkey_ptrs.size()
	);
	if (combined_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_pubkey_combine failed with error {}", combined_result);
	}

	PublicKey serialized_pubkey;
	size_t pubkey_len = serialized_pubkey.size();
	const int serialize_result = secp256k1_ec_pubkey_serialize(m_pContext, serialized_pubkey.data(), &pubkey_len, &publicKey, SECP256K1_EC_COMPRESSED);
	if (serialize_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_pubkey_serialize failed with error {}", serialize_result);
	}

	return serialized_pubkey;
}