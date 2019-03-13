#include "Secp256k1Wrapper.h"

#include "secp256k1-zkp/include/secp256k1_generator.h"
#include "secp256k1-zkp/include/secp256k1_aggsig.h"
#include "secp256k1-zkp/include/secp256k1_commitment.h"
#include "secp256k1-zkp/include/secp256k1_bulletproofs.h"
#include "SwitchGeneratorPoint.h"

#include <Infrastructure/Logger.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Common/Util/FunctionalUtil.h>

const uint64_t MAX_WIDTH = 1 << 20;
const size_t SCRATCH_SPACE_SIZE = 256 * MAX_WIDTH;
const size_t MAX_GENERATORS = 256;

Secp256k1Wrapper& Secp256k1Wrapper::GetInstance()
{
	static Secp256k1Wrapper instance;
	return instance;
}

Secp256k1Wrapper::Secp256k1Wrapper()
{
	m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
	m_pGenerators = secp256k1_bulletproof_generators_create(m_pContext, &secp256k1_generator_const_g, MAX_GENERATORS);
}

Secp256k1Wrapper::~Secp256k1Wrapper()
{
	secp256k1_bulletproof_generators_destroy(m_pContext, m_pGenerators);
	secp256k1_context_destroy(m_pContext);
}

bool Secp256k1Wrapper::VerifySingleAggSig(const Signature& signature, const Commitment& publicKey, const Hash& message) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pedersen_commitment commitment;
	const int commitmentResult = secp256k1_pedersen_commitment_parse(m_pContext, &commitment, publicKey.GetCommitmentBytes().GetData().data());
	if (commitmentResult == 1)
	{
		secp256k1_pubkey pubkey;
		const int pubkeyResult = secp256k1_pedersen_commitment_to_pubkey(m_pContext, &pubkey, &commitment);
		if (pubkeyResult == 1)
		{
			const int verifyResult = secp256k1_aggsig_verify_single(m_pContext, signature.GetSignatureBytes().GetData().data(), message.GetData().data(), nullptr, &pubkey, &pubkey, nullptr, false);
			if (verifyResult == 1)
			{
				return true;
			}
		}
	}

	return false;
}

bool Secp256k1Wrapper::VerifyBulletproofs(const std::vector<std::pair<Commitment, RangeProof>>& rangeProofs) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	const size_t numBits = 64;
	const size_t proofLength = rangeProofs.front().second.GetProofBytes().size();

	// array of generator multiplied by value in pedersen commitments (cannot be NULL)
	std::vector<secp256k1_generator> valueGenerators;
	for (size_t i = 0; i < rangeProofs.size(); i++)
	{
		valueGenerators.push_back(secp256k1_generator_const_h);
	}

	auto getCommitments = [](const std::pair<Commitment, RangeProof>& rangeProof) -> Commitment { return rangeProof.first; };
	std::vector<Commitment> commitments = FunctionalUtil::map<std::vector<Commitment>>(rangeProofs, getCommitments);
	std::vector<secp256k1_pedersen_commitment*> commitmentPointers = ConvertCommitments(commitments);

	std::vector<const unsigned char*> bulletproofPointers;
	bulletproofPointers.reserve(rangeProofs.size());
	for (const std::pair<Commitment, RangeProof>& rangeProof : rangeProofs)
	{
		bulletproofPointers.emplace_back(rangeProof.second.GetProofBytes().data());
	}

	secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(m_pContext, SCRATCH_SPACE_SIZE);
	const int result = secp256k1_bulletproof_rangeproof_verify_multi(m_pContext, pScratchSpace, m_pGenerators, bulletproofPointers.data(), rangeProofs.size(), proofLength, NULL, commitmentPointers.data(), 1, numBits, valueGenerators.data(), NULL, NULL);
	secp256k1_scratch_space_destroy(pScratchSpace);

	CleanupCommitments(commitmentPointers);

	return result == 1;
}

std::unique_ptr<RangeProof> Secp256k1Wrapper::GenerateRangeProof(const uint64_t amount, const BlindingFactor& key, const CBigInteger<32>& nonce, const ProofMessage& proofMessage) const
{
	std::unique_lock<std::shared_mutex> writeLock(m_mutex);

	const CBigInteger<32> randomSeed = RandomNumberGenerator::GenerateRandom32();
	secp256k1_context_randomize(m_pContext, &randomSeed.GetData()[0]);

	std::vector<unsigned char> proofBytes(MAX_PROOF_SIZE, 0);
	size_t proofLen = MAX_PROOF_SIZE;

	/** Produces an aggregate Bulletproof rangeproof for a set of Pedersen commitments
	 *  Returns: 1: rangeproof was successfully created
	 *           0: rangeproof could not be created, or out of memory
	 *  Args:       ctx: pointer to a context object initialized for signing and verification (cannot be NULL)
	 *          scratch: scratch space with enough memory for verification (cannot be NULL)
	 *             gens: generator set with at least 2*nbits*n_commits many generators (cannot be NULL)
	 *  Out:      proof: byte-serialized rangeproof (cannot be NULL)
	 *  In/out:    plen: pointer to size of `proof`, to be replaced with actual length of proof (cannot be NULL)
	 *            tau_x: only for multi-party; 32-byte, output in second step or input in final step
	 *            t_one: only for multi-party; public key, output in first step or input for the others
	 *            t_two: only for multi-party; public key, output in first step or input for the others
	 *  In:       value: array of values committed by the Pedersen commitments (cannot be NULL)
	 *        min_value: array of minimum values to prove ranges above, or NULL for all-zeroes
	 *            blind: array of blinding factors of the Pedersen commitments (cannot be NULL)
	 *          commits: only for multi-party; array of pointers to commitments
	 *        n_commits: number of entries in the `value` and `blind` arrays
	 *        value_gen: generator multiplied by value in pedersen commitments (cannot be NULL)
	 *            nbits: number of bits proven for each range
	 *            nonce: random 32-byte seed used to derive blinding factors (cannot be NULL)
	 *    private_nonce: only for multi-party; random 32-byte seed used to derive private blinding factors
	 *     extra_commit: additonal data committed to by the rangeproof
	 * extra_commit_len: length of additional data
	 *          message: optional 16 bytes of message that can be recovered by rewinding with the correct nonce
	 */
	secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(m_pContext, SCRATCH_SPACE_SIZE);

	std::vector<const unsigned char*> blindingFactors({ key.GetBytes().GetData().data() });
	int result = secp256k1_bulletproof_rangeproof_prove(
		m_pContext, 
		pScratchSpace, 
		m_pGenerators, 
		&proofBytes[0],
		&proofLen, 
		NULL,
		NULL,
		NULL,
		&amount,
		NULL,
		blindingFactors.data(),
		NULL,
		1,
		&secp256k1_generator_const_h,
		64,
		nonce.GetData().data(),
		NULL,
		NULL,
		0,
		proofMessage.GetBytes().GetData().data()
	);
	secp256k1_scratch_space_destroy(pScratchSpace);

	if (result == 1)
	{
		return std::make_unique<RangeProof>(RangeProof(std::move(proofBytes)));
	}

	return std::unique_ptr<RangeProof>(nullptr);
}

std::unique_ptr<RewoundProof> Secp256k1Wrapper::RewindProof(const Commitment& commitment, const RangeProof& rangeProof, const CBigInteger<32>& nonce) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pedersen_commitment*> commitmentPointers = ConvertCommitments(std::vector<Commitment>({ commitment }));

	if (!commitmentPointers.empty())
	{
		uint64_t value;
		std::vector<unsigned char> blindingFactorBytes(32);
		std::vector<unsigned char> message(16);

		int result = secp256k1_bulletproof_rangeproof_rewind(
			m_pContext,
			m_pGenerators,
			&value,
			blindingFactorBytes.data(),
			rangeProof.GetProofBytes().data(),
			rangeProof.GetProofBytes().size(),
			0,
			commitmentPointers.front(),
			&secp256k1_generator_const_h,
			nonce.GetData().data(),
			NULL,
			0,
			message.data()
		);
		CleanupCommitments(commitmentPointers);

		if (result == 1)
		{
			return std::make_unique<RewoundProof>(RewoundProof(value, BlindingFactor(std::move(blindingFactorBytes)), ProofMessage(std::move(message))));
		}
	}

	return std::unique_ptr<RewoundProof>(nullptr);
}

std::unique_ptr<CBigInteger<33>> Secp256k1Wrapper::CalculatePublicKey(const CBigInteger<32>& privateKey) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	const int verifyResult = secp256k1_ec_seckey_verify(m_pContext, privateKey.GetData().data());
	if (verifyResult == 1)
	{
		secp256k1_pubkey pubkey;
		const int createResult = secp256k1_ec_pubkey_create(m_pContext, &pubkey, privateKey.GetData().data());
		if (createResult == 1)
		{
			size_t length = 33;
			std::vector<unsigned char> serializedPublicKey(length);
			const int serializeResult = secp256k1_ec_pubkey_serialize(m_pContext, serializedPublicKey.data(), &length, &pubkey, SECP256K1_EC_COMPRESSED);
			if (serializeResult == 1)
			{
				return std::make_unique<CBigInteger<33>>(CBigInteger<33>(std::move(serializedPublicKey)));
			}
		}
	}

	return std::unique_ptr<CBigInteger<33>>(nullptr);
}

std::unique_ptr<CBigInteger<33>> Secp256k1Wrapper::PublicKeySum(const std::vector<CBigInteger<33>>& publicKeys) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pubkey*> parsedPubKeys;
	for (const CBigInteger<33>& publicKey : publicKeys)
	{
		secp256k1_pubkey* pPublicKey = new secp256k1_pubkey();
		int pubKeyParsed = secp256k1_ec_pubkey_parse(m_pContext, pPublicKey, &publicKey.GetData()[0], publicKey.GetData().size());
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

			return std::unique_ptr<CBigInteger<33>>(nullptr);
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
			return std::make_unique<CBigInteger<33>>(CBigInteger<33>(std::move(serializedPublicKey)));
		}
	}

	return std::unique_ptr<CBigInteger<33>>(nullptr);
}

std::unique_ptr<Commitment> Secp256k1Wrapper::PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pedersen_commitment commitment;
	const int result = secp256k1_pedersen_commit(m_pContext, &commitment, &blindingFactor.GetBytes()[0], value, &secp256k1_generator_const_h, &secp256k1_generator_const_g);
	if (result == 1)
	{
		std::vector<unsigned char> serializedCommitment(33);
		secp256k1_pedersen_commitment_serialize(m_pContext, &serializedCommitment[0], &commitment);

		return std::make_unique<Commitment>(Commitment(CBigInteger<33>(std::move(serializedCommitment))));
	}

	LoggerAPI::LogError("Secp256k1Wrapper::PedersenCommit - Failed to create commitment. Result: " + std::to_string(result) + " Value: " + std::to_string(value) + " Blind: " + blindingFactor.GetBytes().ToHex());
	return std::unique_ptr<Commitment>(nullptr);
}

std::unique_ptr<Commitment> Secp256k1Wrapper::PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pedersen_commitment*> positiveCommitments = ConvertCommitments(positive);
	std::vector<secp256k1_pedersen_commitment*> negativeCommitments = ConvertCommitments(negative);

	secp256k1_pedersen_commitment commitment;
	const int result = secp256k1_pedersen_commit_sum(
		m_pContext, 
		&commitment, 
		positiveCommitments.empty() ? nullptr : &positiveCommitments[0],
		positiveCommitments.size(), 
		negativeCommitments.empty() ? nullptr : &negativeCommitments[0], 
		negativeCommitments.size()
	);

	CleanupCommitments(positiveCommitments);
	CleanupCommitments(negativeCommitments);

	if (result == 1)
	{
		std::vector<unsigned char> serializedCommitment(33);
		secp256k1_pedersen_commitment_serialize(m_pContext, &serializedCommitment[0], &commitment);

		return std::make_unique<Commitment>(Commitment(CBigInteger<33>(std::move(serializedCommitment))));
	}

	LoggerAPI::LogError("Secp256k1Wrapper::PedersenCommitSum - secp256k1_pedersen_commit_sum returned result: " + std::to_string(result));
	return std::unique_ptr<Commitment>(nullptr);
}

std::unique_ptr<BlindingFactor> Secp256k1Wrapper::PedersenBlindSum(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<const unsigned char*> blindingFactors;
	for (const BlindingFactor& positiveFactor : positive)
	{
		blindingFactors.push_back(positiveFactor.GetBytes().GetData().data());
	}

	for (const BlindingFactor& negativeFactor : negative)
	{
		blindingFactors.push_back(negativeFactor.GetBytes().GetData().data());
	}

	CBigInteger<32> blindingFactorBytes;
	const int result = secp256k1_pedersen_blind_sum(m_pContext, &blindingFactorBytes[0], blindingFactors.data(), blindingFactors.size(), positive.size());

	if (result == 1)
	{
		return std::make_unique<BlindingFactor>(BlindingFactor(std::move(blindingFactorBytes)));
	}

	LoggerAPI::LogError("Secp256k1Wrapper::PedersenBlindSum - secp256k1_pedersen_blind_sum returned result: " + std::to_string(result));
	return std::unique_ptr<BlindingFactor>(nullptr);
}

std::vector<secp256k1_pedersen_commitment*> Secp256k1Wrapper::ConvertCommitments(const std::vector<Commitment>& commitments) const
{
	std::vector<secp256k1_pedersen_commitment*> convertedCommitments(commitments.size(), NULL);
	for (int i = 0; i < commitments.size(); i++)
	{
		const std::vector<unsigned char>& commitmentBytes = commitments[i].GetCommitmentBytes().GetData();

		secp256k1_pedersen_commitment* pCommitment = new secp256k1_pedersen_commitment();
		const int parsed = secp256k1_pedersen_commitment_parse(m_pContext, pCommitment, &commitmentBytes[0]);
		convertedCommitments[i] = pCommitment;

		if (parsed != 1)
		{
			// TODO: Log failue
			CleanupCommitments(convertedCommitments);
			return std::vector<secp256k1_pedersen_commitment*>();
		}
	}

	return convertedCommitments;
}

void Secp256k1Wrapper::CleanupCommitments(std::vector<secp256k1_pedersen_commitment*>& commitments) const
{
	for (secp256k1_pedersen_commitment* pCommitment : commitments)
	{
		delete pCommitment;
	}

	commitments.clear();
}

std::unique_ptr<Signature> Secp256k1Wrapper::SignSingle(const BlindingFactor& secretKey, const BlindingFactor& secretNonce, const CBigInteger<33>& sumPubKeys, const CBigInteger<33>& sumPubNonces, const Hash& message) const
{
	std::lock_guard<std::shared_mutex> writeLock(m_mutex);

	const CBigInteger<32> randomSeed = RandomNumberGenerator::GenerateRandom32();
	secp256k1_context_randomize(m_pContext, &randomSeed.GetData()[0]);

	secp256k1_pubkey pubKeyForE;
	int pubKeyParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubKeyForE, &sumPubKeys.GetData()[0], sumPubKeys.GetData().size());

	secp256k1_pubkey pubNoncesForE;
	int noncesParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubNoncesForE, &sumPubNonces.GetData()[0], sumPubNonces.GetData().size());

	if (pubKeyParsed == 1 && noncesParsed == 1)
	{
		std::vector<unsigned char> signatureBytes(64);
		int signedResult = secp256k1_aggsig_sign_single(
			m_pContext,
			&signatureBytes[0],
			&message.GetData()[0],
			&secretKey.GetBytes().GetData()[0],
			&secretNonce.GetBytes().GetData()[0],
			nullptr,
			&pubNoncesForE,
			nullptr,
			&pubKeyForE,
			nullptr
		);

		if (signedResult == 1)
		{
			return std::make_unique<Signature>(Signature(CBigInteger<64>(std::move(signatureBytes))));
		}
	}

	return std::unique_ptr<Signature>(nullptr);
}

std::unique_ptr<Signature> Secp256k1Wrapper::AggregateSignatures(const std::vector<Signature>& signatures, const CBigInteger<33>& sumPubNonces) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pubkey pubNonces;
	int noncesParsed = secp256k1_ec_pubkey_parse(m_pContext, &pubNonces, &sumPubNonces.GetData()[0], sumPubNonces.GetData().size());
	if (noncesParsed == 1)
	{
		std::vector<const unsigned char*> signaturePointers;
		for (const Signature signature : signatures)
		{
			signaturePointers.push_back(signature.GetSignatureBytes().GetData().data());
		}

		std::vector<unsigned char> signatureBytes(64);
		int result = secp256k1_aggsig_add_signatures_single(m_pContext, signatureBytes.data(), signaturePointers.data(), signaturePointers.size(), &pubNonces);
		if (result == 1)
		{
			return std::make_unique<Signature>(Signature(CBigInteger<64>(std::move(signatureBytes))));
		}
	}

	return std::unique_ptr<Signature>(nullptr);
}

std::unique_ptr<BlindingFactor> Secp256k1Wrapper::BlindSwitch(const BlindingFactor& blindingFactor, const uint64_t amount) const
{
	std::shared_lock<std::shared_mutex> writeLock(m_mutex);

	std::vector<unsigned char> blindSwitch(32);
	const int result = secp256k1_blind_switch(m_pContext, blindSwitch.data(), blindingFactor.GetBytes().GetData().data(), amount, &secp256k1_generator_const_h, &secp256k1_generator_const_g, &GENERATOR_J_PUB);
	if (result == 1)
	{
		return std::make_unique<BlindingFactor>(BlindingFactor(CBigInteger<32>(std::move(blindSwitch))));
	}

	return std::unique_ptr<BlindingFactor>(nullptr);
}