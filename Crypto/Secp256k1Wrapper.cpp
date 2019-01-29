#include "Secp256k1Wrapper.h"

#include "secp256k1-zkp/include/secp256k1_generator.h"
#include "secp256k1-zkp/include/secp256k1_aggsig.h"
#include "secp256k1-zkp/include/secp256k1_commitment.h"
#include "secp256k1-zkp/include/secp256k1_bulletproofs.h"

#include <Infrastructure/Logger.h>
#include <Crypto/RandomNumberGenerator.h>

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

bool Secp256k1Wrapper::VerifyBulletproofs(const std::vector<Commitment>& commitments, const std::vector<RangeProof>& rangeProofs) const
{
	const size_t numBits = 64;

	const size_t proofLength = rangeProofs.front().GetProofBytes().size();

	// array of generator multiplied by value in pedersen commitments (cannot be NULL)
	std::vector<secp256k1_generator> valueGenerators;
	for (size_t i = 0; i < rangeProofs.size(); i++)
	{
		valueGenerators.push_back(secp256k1_generator_const_h);
	}


	std::vector<secp256k1_pedersen_commitment*> commitmentPointers = ConvertCommitments(commitments);
	std::vector<const unsigned char*> bulletproofPointers;
	for (const RangeProof& rangeProof : rangeProofs)
	{
		bulletproofPointers.push_back(rangeProof.GetProofBytes().data());
	}

	secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(m_pContext, SCRATCH_SPACE_SIZE);

	const int result = secp256k1_bulletproof_rangeproof_verify_multi(m_pContext, pScratchSpace, m_pGenerators, bulletproofPointers.data(), rangeProofs.size(), proofLength, NULL, commitmentPointers.data(), 1, numBits, valueGenerators.data(), NULL, NULL);

	secp256k1_scratch_space_destroy(pScratchSpace);

	CleanupCommitments(commitmentPointers);

	return result == 1;
}

std::unique_ptr<CBigInteger<33>> Secp256k1Wrapper::CalculatePublicKey(const CBigInteger<32>& privateKey) const
{
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

std::unique_ptr<Commitment> Secp256k1Wrapper::PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const
{
	const CBigInteger<32> randomSeed = RandomNumberGenerator().GeneratePseudoRandomNumber(CBigInteger<32>::ValueOf(0), CBigInteger<32>::GetMaximumValue());
	secp256k1_context_randomize(m_pContext, &randomSeed.GetData()[0]);

	secp256k1_pedersen_commitment commitment;
	const int result = secp256k1_pedersen_commit(m_pContext, &commitment, &blindingFactor.GetBlindingFactorBytes()[0], value, &secp256k1_generator_const_h, &secp256k1_generator_const_g);
	if (result == 1)
	{
		std::vector<unsigned char> serializedCommitment(33);
		secp256k1_pedersen_commitment_serialize(m_pContext, &serializedCommitment[0], &commitment);

		return std::make_unique<Commitment>(Commitment(CBigInteger<33>(std::move(serializedCommitment))));
	}

	LoggerAPI::LogError("Secp256k1Wrapper::PedersenCommit - Failed to create commitment. Result: " + std::to_string(result) + " Value: " + std::to_string(value) + " Blind: " + blindingFactor.GetBlindingFactorBytes().ToHex());
	return std::unique_ptr<Commitment>(nullptr);
}

std::unique_ptr<Commitment> Secp256k1Wrapper::PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const
{
	const CBigInteger<32> randomSeed = RandomNumberGenerator().GeneratePseudoRandomNumber(CBigInteger<32>::ValueOf(0), CBigInteger<32>::GetMaximumValue());
	secp256k1_context_randomize(m_pContext, &randomSeed.GetData()[0]);

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
	const CBigInteger<32> randomSeed = RandomNumberGenerator().GeneratePseudoRandomNumber(CBigInteger<32>::ValueOf(0), CBigInteger<32>::GetMaximumValue());
	secp256k1_context_randomize(m_pContext, &randomSeed.GetData()[0]);

	std::vector<const unsigned char*> blindingFactors;
	for (const BlindingFactor& positiveFactor : positive)
	{
		blindingFactors.push_back(positiveFactor.GetBlindingFactorBytes().GetData().data());
	}

	for (const BlindingFactor& negativeFactor : negative)
	{
		blindingFactors.push_back(negativeFactor.GetBlindingFactorBytes().GetData().data());
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