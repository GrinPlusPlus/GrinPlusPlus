#include "AggSig.h"

#include "secp256k1-zkp/include/secp256k1_generator.h"
#include "secp256k1-zkp/include/secp256k1_aggsig.h"
#include "secp256k1-zkp/include/secp256k1_commitment.h"
#include "secp256k1-zkp/include/secp256k1_schnorrsig.h"
#include "Pedersen.h"

#include <Infrastructure/Logger.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Crypto/CryptoException.h>

const uint64_t MAX_WIDTH = 1 << 20;
const size_t SCRATCH_SPACE_SIZE = 256 * MAX_WIDTH; // TODO: Determine actual size

AggSig& AggSig::GetInstance()
{
	static AggSig instance;
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

std::unique_ptr<SecretKey> AggSig::GenerateSecureNonce() const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<unsigned char> nonce(32);
	const SecretKey seed = RandomNumberGenerator::GenerateRandom32();

	const int result = secp256k1_aggsig_export_secnonce_single(m_pContext, nonce.data(), seed.data());
	if (result == 1)
	{
		return std::make_unique<SecretKey>(CBigInteger<32>(std::move(nonce)));
	}

	return std::unique_ptr<SecretKey>(nullptr);
}

std::unique_ptr<Signature> AggSig::SignMessage(const SecretKey& secretKey, const PublicKey& publicKey, const Hash& message)
{
	std::lock_guard<std::shared_mutex> writeLock(m_mutex);

	const SecretKey randomSeed = RandomNumberGenerator::GenerateRandom32();
	const int randomizeResult = secp256k1_context_randomize(m_pContext, randomSeed.data());
	if (randomizeResult != 1)
    {
	    LoggerAPI::LogError("AggSig::SignMessage - Context randomization failed.");
        return std::unique_ptr<Signature>(nullptr);
    }

	secp256k1_pubkey pubKey;
	int pubKeyParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubKey, publicKey.data(), publicKey.size());

	if (pubKeyParsed == 1)
	{
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

		if (signedResult == 1)
		{
			std::vector<unsigned char> signatureBytes(64);
			const int serializedResult = secp256k1_ecdsa_signature_serialize_compact(m_pContext, signatureBytes.data(), &signature);
			if (serializedResult == 1)
			{
				return std::make_unique<Signature>(Signature(CBigInteger<64>(std::move(signatureBytes))));
			}
		}
	}

	return std::unique_ptr<Signature>(nullptr);
}

bool AggSig::VerifyMessageSignature(const Signature& signature, const PublicKey& publicKey, const Hash& message) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_ecdsa_signature secpSig;
	const int parseSignatureResult = secp256k1_ecdsa_signature_parse_compact(m_pContext, &secpSig, signature.GetSignatureBytes().data());
	if (parseSignatureResult == 1)
	{
		secp256k1_pubkey pubkey;
		const int pubkeyResult = secp256k1_ec_pubkey_parse(m_pContext, &pubkey, publicKey.data(), publicKey.size());

		if (pubkeyResult == 1)
		{
			const int verifyResult = secp256k1_aggsig_verify_single(m_pContext, /*signature.GetSignatureBytes().data()*/secpSig.data, message.data(), nullptr, &pubkey, &pubkey, nullptr, false);
			if (verifyResult == 1)
			{
				return true;
			}
		}
	}

	return false;
}

std::unique_ptr<Signature> AggSig::CalculatePartialSignature(const SecretKey& secretKey, const SecretKey& secretNonce, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message)
{
	std::lock_guard<std::shared_mutex> writeLock(m_mutex);

	const SecretKey randomSeed = RandomNumberGenerator::GenerateRandom32();
	const int randomizeResult = secp256k1_context_randomize(m_pContext, randomSeed.data());
    if (randomizeResult != 1)
    {
        LoggerAPI::LogError("AggSig::SignMessage - Context randomization failed.");
        return std::unique_ptr<Signature>(nullptr);
    }

	secp256k1_pubkey pubKeyForE;
	int pubKeyParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubKeyForE, sumPubKeys.data(), sumPubKeys.size());

	secp256k1_pubkey pubNoncesForE;
	int noncesParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubNoncesForE, sumPubNonces.data(), sumPubNonces.size());

	if (pubKeyParsed == 1 && noncesParsed == 1)
	{
		secp256k1_ecdsa_signature signature;
		const int signedResult = secp256k1_aggsig_sign_single(
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

		if (signedResult == 1)
		{
			std::vector<unsigned char> signatureBytes(64);
			const int serializedResult = secp256k1_ecdsa_signature_serialize_compact(m_pContext, signatureBytes.data(), &signature);
			if (serializedResult == 1)
			{
				return std::make_unique<Signature>(Signature(CBigInteger<64>(std::move(signatureBytes))));
			}
		}
	}

	return std::unique_ptr<Signature>(nullptr);
}

bool AggSig::VerifyPartialSignature(const Signature& partialSignature, const PublicKey& publicKey, const PublicKey& sumPubKeys, const PublicKey& sumPubNonces, const Hash& message) const
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

std::unique_ptr<Signature> AggSig::AggregateSignatures(const std::vector<Signature>& signatures, const PublicKey& sumPubNonces) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pubkey pubNonces;
	int noncesParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubNonces, sumPubNonces.data(), sumPubNonces.size());
	if (noncesParsed != 1)
	{
		return std::unique_ptr<Signature>();
	}

	std::vector<secp256k1_ecdsa_signature> parsedSignatures = ParseSignatures(signatures);
	if (parsedSignatures.empty())
	{
		return std::unique_ptr<Signature>();
	}

	std::vector<secp256k1_ecdsa_signature*> signaturePointers;
	for (secp256k1_ecdsa_signature& parsedSignature : parsedSignatures)
	{
		signaturePointers.push_back(&parsedSignature);
	}

	secp256k1_ecdsa_signature aggregatedSignature;
	const int result = secp256k1_aggsig_add_signatures_single(m_pContext, aggregatedSignature.data, (const unsigned char**)signaturePointers.data(), signaturePointers.size(), &pubNonces);
	if (result == 1)
	{
		return std::make_unique<Signature>(Signature(CBigInteger<64>(std::move(aggregatedSignature.data))));
		//return SerializeSignature(aggregatedSignature);
	}

	return std::unique_ptr<Signature>(nullptr);
}

bool AggSig::VerifyAggregateSignatures(const std::vector<const Signature*>& signatures, const std::vector<const Commitment*>& commitments, const std::vector<const Hash*>& messages) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pubkey> parsedPubKeys;
	for (const Commitment* commitment : commitments)
	{
		secp256k1_pedersen_commitment parsedCommitment;
		const int commitmentResult = secp256k1_pedersen_commitment_parse(m_pContext, &parsedCommitment, commitment->GetCommitmentBytes().data());
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
				LoggerAPI::LogError("AggSig::VerifyAggregateSignatures - Failed to convert commitment to pubkey: " + commitment->ToHex());
				return false;
			}
		}
		else
		{
			LoggerAPI::LogError("AggSig::VerifyAggregateSignatures - Failed to parse commitment " + commitment->ToHex());
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
	for (secp256k1_schnorrsig& parsedSig : parsedSignatures)
	{
		signaturePtrs.push_back(&parsedSig);
	}

	std::vector<const unsigned char*> messageData;
	for (const Hash* message : messages)
	{
		messageData.emplace_back(message->data());
	}

	secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(m_pContext, SCRATCH_SPACE_SIZE);
	const int verifyResult = secp256k1_schnorrsig_verify_batch(m_pContext, pScratchSpace, signaturePtrs.data(), messageData.data(), pubKeyPtrs.data(), signatures.size());
	secp256k1_scratch_space_destroy(pScratchSpace);

	if (verifyResult == 1)
	{
		return true;
	}
	else
	{
		LoggerAPI::LogError("AggSig::VerifyAggregateSignatures - Signature failed to verify.");
		return false;
	}
}

bool AggSig::VerifyAggregateSignature(const Signature& signature, const Commitment& commitment, const Hash& message) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pedersen_commitment parsedCommitment;
	const int commitmentResult = secp256k1_pedersen_commitment_parse(m_pContext, &parsedCommitment, commitment.GetCommitmentBytes().data());
	if (commitmentResult == 1)
	{
		secp256k1_pubkey pubkey;
		const int pubkeyResult = secp256k1_pedersen_commitment_to_pubkey(m_pContext, &pubkey, &parsedCommitment);
		if (pubkeyResult == 1)
		{
			const int verifyResult = secp256k1_aggsig_verify_single(m_pContext, signature.GetSignatureBytes().data(), message.data(), nullptr, &pubkey, &pubkey, nullptr, false);
			if (verifyResult == 1)
			{
				return true;
			}
		}
	}

	return false;
}

bool AggSig::VerifyAggregateSignature(const Signature& signature, const PublicKey& sumPubKeys, const Hash& message) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pubkey parsedPubKey;
	const int parseResult = secp256k1_ec_pubkey_parse(m_pContext, &parsedPubKey, sumPubKeys.data(), sumPubKeys.size());
	if (parseResult == 1)
	{
		//secp256k1_ecdsa_signature parsedSignature;
		//const int parseSignatureResult = secp256k1_ecdsa_signature_parse_compact(m_pContext, &parsedSignature, signature.GetSignatureBytes().data());
		//if (parseSignatureResult == 1)
		//{
		const int verifyResult = secp256k1_aggsig_verify_single(m_pContext, signature.GetSignatureBytes().data(), message.data(), nullptr, &parsedPubKey, &parsedPubKey, nullptr, false);
		if (verifyResult == 1)
		{
			return true;
		}
		//}
	}

	return false;
}

std::vector<secp256k1_ecdsa_signature> AggSig::ParseSignatures(const std::vector<Signature>& signatures) const
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

std::unique_ptr<Signature> AggSig::SerializeSignature(const secp256k1_ecdsa_signature& signature) const
{
	std::vector<unsigned char> signatureBytes(64);
	const int serializedResult = secp256k1_ecdsa_signature_serialize_compact(m_pContext, signatureBytes.data(), &signature);
	if (serializedResult == 1)
	{
		return std::make_unique<Signature>(Signature(CBigInteger<64>(std::move(signatureBytes))));
	}

	return std::unique_ptr<Signature>(nullptr);
}

Signature AggSig::ConvertToRaw(const Signature& compressed) const
{
	std::vector<unsigned char> raw(64);
	const int parseSignatureResult = secp256k1_ecdsa_signature_parse_compact(m_pContext, (secp256k1_ecdsa_signature*)raw.data(), compressed.GetSignatureBytes().data());
	if (parseSignatureResult == 1)
	{
		return Signature(CBigInteger<64>(std::move(raw)));
	}

	throw CryptoException();
}

Signature AggSig::ConvertToCompressed(const Signature& raw) const
{
	std::vector<unsigned char> compressed(64);
	const int parseSignatureResult = secp256k1_ecdsa_signature_serialize_compact(m_pContext, compressed.data(), (secp256k1_ecdsa_signature*)raw.GetSignatureBytes().data());
	if (parseSignatureResult == 1)
	{
		return Signature(CBigInteger<64>(std::move(compressed)));
	}

	throw CryptoException();
}