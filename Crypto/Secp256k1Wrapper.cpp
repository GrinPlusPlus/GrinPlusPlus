#include "Secp256k1Wrapper.h"

#include "RandomNumberGenerator.h"
#include "secp256k1-zkp/include/secp256k1_generator.h"

Secp256k1Wrapper& Secp256k1Wrapper::GetInstance()
{
	static Secp256k1Wrapper instance;
	return instance;
}

Secp256k1Wrapper::Secp256k1Wrapper()
{
	m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
}

Secp256k1Wrapper::~Secp256k1Wrapper()
{
	secp256k1_context_destroy(m_pContext);
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

	// TODO: Log Failure
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

	// TODO: Log failue
	return std::unique_ptr<Commitment>(nullptr);
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
		delete pCommitment; // TODO: delete or delete[]?
	}

	commitments.clear();
}