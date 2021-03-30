#include "AggSig.h"
#include "Pedersen.h"

#include <secp256k1-zkp/secp256k1_generator.h>
#include <secp256k1-zkp/secp256k1_aggsig.h>
#include <secp256k1-zkp/secp256k1_commitment.h>
#include <secp256k1-zkp/secp256k1_schnorrsig.h>
#include <Common/Logger.h>
#include <Crypto/CSPRNG.h>
#include <Core/Exceptions/CryptoException.h>

const uint64_t MAX_WIDTH = 1 << 20;
const size_t SCRATCH_SPACE_SIZE = 256 * MAX_WIDTH;

static AggSig instance;

AggSig& AggSig::GetInstance()
{
	return instance;
}

AggSig::AggSig()
{
	m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
}

AggSig::~AggSig()
{
	secp256k1_context_destroy(m_pContext);
}

SecretKey AggSig::GenerateSecureNonce() const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<unsigned char> nonce(32);
	const SecretKey seed = CSPRNG::GenerateRandom32();

	const int result = secp256k1_aggsig_export_secnonce_single(m_pContext, nonce.data(), seed.data());
	if (result == 1)
	{
		return SecretKey(CBigInteger<32>(std::move(nonce)));
	}

	throw CryptoException("secp256k1_aggsig_export_secnonce_single failed with error: " + std::to_string(result));
}

std::unique_ptr<Signature> AggSig::BuildSignature(const SecretKey& secretKey, const Commitment& commitment, const Hash& message)
{
	std::lock_guard<std::shared_mutex> writeLock(m_mutex);

	const SecretKey randomSeed = CSPRNG::GenerateRandom32();
	const int randomizeResult = secp256k1_context_randomize(m_pContext, randomSeed.data());
	if (randomizeResult != 1)
	{
		LOG_ERROR("Context randomization failed");
		return nullptr;
	}

	secp256k1_pedersen_commitment parsedCommitment;
	const int commitmentResult = secp256k1_pedersen_commitment_parse(m_pContext, &parsedCommitment, commitment.data());
	if (commitmentResult != 1)
	{
		LOG_ERROR("secp256k1_pedersen_commitment_parse failed");
		return nullptr;
	}

	secp256k1_pubkey pubKey;
	const int pubkeyResult = secp256k1_pedersen_commitment_to_pubkey(m_pContext, &pubKey, &parsedCommitment);
	if (pubkeyResult != 1)
	{
		LOG_ERROR("secp256k1_pedersen_commitment_to_pubkey failed");
		return nullptr;
	}

	secp256k1_ecdsa_signature signature;
	const int signedResult = secp256k1_aggsig_sign_single(
		m_pContext,
		&signature.data[0],
		message.data(),
		secretKey.data(),
		nullptr,
		nullptr,
		nullptr,
		nullptr,
		&pubKey,
		randomSeed.data()
	);

	if (signedResult != 1)
	{
		LOG_ERROR("secp256k1_aggsig_sign_single failed");
		return nullptr;
	}

	return std::make_unique<Signature>(CBigInteger<64>(signature.data));
}

CompactSignature AggSig::CalculatePartialSignature(const SecretKey& secretKey, const SecretKey& secretNonce, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message)
{
	std::lock_guard<std::shared_mutex> writeLock(m_mutex);

	const SecretKey randomSeed = CSPRNG::GenerateRandom32();
	const int randomize_result = secp256k1_context_randomize(m_pContext, randomSeed.data());
    if (randomize_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_context_randomize failed with error {}", randomize_result);
    }

	secp256k1_pubkey pubKeyForE;
	int pubkey_parsed = secp256k1_ec_pubkey_parse(m_pContext, &pubKeyForE, sumPubKeys.data(), sumPubKeys.size());
	if (pubkey_parsed != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_pubkey_parse failed with error {}", pubkey_parsed);
	}

	secp256k1_pubkey pubNoncesForE;
	int nonces_parsed = secp256k1_ec_pubkey_parse(m_pContext, &pubNoncesForE, sumPubNonces.data(), sumPubNonces.size());
	if (nonces_parsed != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ec_pubkey_parse failed with error {}", nonces_parsed);
	}

	secp256k1_ecdsa_signature signature;
	const int signed_result = secp256k1_aggsig_sign_single(
		m_pContext,
		&signature.data[0],
		message.data(),
		secretKey.data(),
		secretNonce.data(),
		nullptr,
		&pubNoncesForE,
		&pubNoncesForE,
		&pubKeyForE,
		randomSeed.data()
	);
	if (signed_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_aggsig_sign_single failed with error {}", signed_result);
	}

	CompactSignature result;
	const int serialized_result = secp256k1_ecdsa_signature_serialize_compact(m_pContext, result.data(), &signature);
	if (serialized_result != 1) {
		throw CRYPTO_EXCEPTION_F("secp256k1_ecdsa_signature_serialize_compact failed with error {}", serialized_result);
	}

	return result;
}

bool AggSig::VerifyPartialSignature(const CompactSignature& partialSignature, const PublicKey& publicKey, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_ecdsa_signature signature;
	
	const int parseSignatureResult = secp256k1_ecdsa_signature_parse_compact(m_pContext, &signature, partialSignature.GetSignatureBytes().data());
	if (parseSignatureResult == 1)
	{
		secp256k1_pubkey pubkey;
		const int pubkeyResult = secp256k1_ec_pubkey_parse(m_pContext, &pubkey, publicKey.data(), publicKey.size());

		secp256k1_pubkey sumPubKey;
		const int sumPubkeysResult = secp256k1_ec_pubkey_parse(m_pContext, &sumPubKey, sumPubKeys.data(), sumPubKeys.size());

		secp256k1_pubkey sumNoncesPubKey;
		const int sumPubNonceKeyResult = secp256k1_ec_pubkey_parse(m_pContext, &sumNoncesPubKey, sumPubNonces.data(), sumPubNonces.size());

		if (pubkeyResult == 1 && sumPubkeysResult == 1 && sumPubNonceKeyResult == 1)
		{
			const int verifyResult = secp256k1_aggsig_verify_single(m_pContext, signature.data, message.data(), &sumNoncesPubKey, &pubkey, &sumPubKey, nullptr, true);
			if (verifyResult == 1)
			{
				return true;
			}
		}
	}

	return false;
}

std::unique_ptr<Signature> AggSig::AggregateSignatures(const std::vector<CompactSignature>& signatures, const PublicKey& sumPubNonces) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pubkey pubNonces;
	const int noncesParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubNonces, sumPubNonces.data(), sumPubNonces.size());
	if (noncesParsed == 1)
	{
		std::vector<secp256k1_ecdsa_signature> parsedSignatures = ParseCompactSignatures(signatures);
		if (!parsedSignatures.empty())
		{
			std::vector<secp256k1_ecdsa_signature*> signaturePointers;
			std::transform(
				parsedSignatures.begin(),
				parsedSignatures.end(),
				std::back_inserter(signaturePointers),
				[](secp256k1_ecdsa_signature& parsedSig) { return &parsedSig; }
			);

			secp256k1_ecdsa_signature aggregatedSignature;
			const int result = secp256k1_aggsig_add_signatures_single(
				m_pContext, 
				aggregatedSignature.data, 
				(const unsigned char**)signaturePointers.data(), 
				signaturePointers.size(), 
				&pubNonces
			);

			if (result == 1)
			{
				return std::make_unique<Signature>(Signature(CBigInteger<64>(aggregatedSignature.data)));
			}
		}
	}

	LOG_ERROR("Failed to aggregate signatures");
	return std::unique_ptr<Signature>(nullptr);
}

bool AggSig::VerifyAggregateSignatures(const std::vector<const Signature*>& signatures, const std::vector<const Commitment*>& commitments, const std::vector<const Hash*>& messages) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pubkey> parsedPubKeys;
	for (const Commitment* commitment : commitments)
	{
		secp256k1_pedersen_commitment parsedCommitment;
		const int commitmentResult = secp256k1_pedersen_commitment_parse(m_pContext, &parsedCommitment, commitment->data());
		if (commitmentResult == 1)
		{
			secp256k1_pubkey pubKey;
			const int pubkeyResult = secp256k1_pedersen_commitment_to_pubkey(m_pContext, &pubKey, &parsedCommitment);
			if (pubkeyResult == 1)
			{
				parsedPubKeys.emplace_back(std::move(pubKey));
			}
			else
			{
				LOG_ERROR("Failed to convert commitment to pubkey: " + commitment->ToHex());
				return false;
			}
		}
		else
		{
			LOG_ERROR("Failed to parse commitment " + commitment->ToHex());
			return false;
		}
	}

	std::vector<secp256k1_pubkey*> pubKeyPtrs(parsedPubKeys.size());
	for (size_t i = 0; i < parsedPubKeys.size(); i++)
	{
		pubKeyPtrs[i] = &parsedPubKeys[i];
	}

	std::vector<secp256k1_schnorrsig> parsedSignatures;
	for (const Signature* signature : signatures)
	{
		secp256k1_schnorrsig parsedSig;
		if (secp256k1_schnorrsig_parse(m_pContext, &parsedSig, signature->GetSignatureBytes().data()) == 0)
		{
			return false;
		}

		parsedSignatures.emplace_back(std::move(parsedSig));
	}

	std::vector<const secp256k1_schnorrsig*> signaturePtrs;
	std::transform(
		parsedSignatures.cbegin(),
		parsedSignatures.cend(),
		std::back_inserter(signaturePtrs),
		[](const secp256k1_schnorrsig& parsedSig) { return &parsedSig; }
	);

	std::vector<const unsigned char*> messageData;
	std::transform(
		messages.cbegin(),
		messages.cend(),
		std::back_inserter(messageData),
		[](const Hash* pMessage) { return pMessage->data(); }
	);

	secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(m_pContext, SCRATCH_SPACE_SIZE);
	const int verifyResult = secp256k1_schnorrsig_verify_batch(m_pContext, pScratchSpace, signaturePtrs.data(), messageData.data(), pubKeyPtrs.data(), signatures.size());
	secp256k1_scratch_space_destroy(pScratchSpace);

	if (verifyResult == 1)
	{
		return true;
	}
	else
	{
		LOG_ERROR("Signature failed to verify.");
		return false;
	}
}

bool AggSig::VerifyAggregateSignature(const Signature& signature, const PublicKey& sumPubKeys, const Hash& message) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pubkey parsedPubKey;
	const int parseResult = secp256k1_ec_pubkey_parse(m_pContext, &parsedPubKey, sumPubKeys.data(), sumPubKeys.size());
	if (parseResult == 1)
	{
		const int verifyResult = secp256k1_aggsig_verify_single(m_pContext, signature.GetSignatureBytes().data(), message.data(), nullptr, &parsedPubKey, &parsedPubKey, nullptr, false);
		if (verifyResult == 1)
		{
			return true;
		}
	}

	return false;
}

std::vector<secp256k1_ecdsa_signature> AggSig::ParseCompactSignatures(const std::vector<CompactSignature>& signatures) const
{
	std::vector<secp256k1_ecdsa_signature> parsed;
	for (const Signature partialSignature : signatures)
	{
		secp256k1_ecdsa_signature signature;
		const int parseSignatureResult = secp256k1_ecdsa_signature_parse_compact(m_pContext, &signature, partialSignature.GetSignatureBytes().data());
		if (parseSignatureResult == 1)
		{
			parsed.emplace_back(std::move(signature));
		}
		else
		{
			return std::vector<secp256k1_ecdsa_signature>();
		}
	}

	return parsed;
}

CompactSignature AggSig::ToCompact(const Signature& signature) const
{
	CompactSignature compactSignature;
	secp256k1_ecdsa_signature ecdsa_sig;
	memcpy(ecdsa_sig.data, signature.data(), 64);
	const int result = secp256k1_ecdsa_signature_serialize_compact(m_pContext, compactSignature.data(), &ecdsa_sig);
	if (result != 1) {
		throw CryptoException("Failed to serialize to compact");
	}

	return compactSignature;
}