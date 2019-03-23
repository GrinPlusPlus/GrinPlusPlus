#include "Pedersen.h"

#include "secp256k1-zkp/include/secp256k1_commitment.h"
#include "SwitchGeneratorPoint.h"

#include <Infrastructure/Logger.h>

Pedersen& Pedersen::GetInstance()
{
	static Pedersen instance;
	return instance;
}

Pedersen::Pedersen()
{
	m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
}

Pedersen::~Pedersen()
{
	secp256k1_context_destroy(m_pContext);
}

std::unique_ptr<Commitment> Pedersen::PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const
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

std::unique_ptr<Commitment> Pedersen::PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pedersen_commitment*> positiveCommitments = Pedersen::ConvertCommitments(*m_pContext, positive);
	std::vector<secp256k1_pedersen_commitment*> negativeCommitments = Pedersen::ConvertCommitments(*m_pContext, negative);

	secp256k1_pedersen_commitment commitment;
	const int result = secp256k1_pedersen_commit_sum(
		m_pContext,
		&commitment,
		positiveCommitments.empty() ? nullptr : &positiveCommitments[0],
		positiveCommitments.size(),
		negativeCommitments.empty() ? nullptr : &negativeCommitments[0],
		negativeCommitments.size()
	);

	Pedersen::CleanupCommitments(positiveCommitments);
	Pedersen::CleanupCommitments(negativeCommitments);

	if (result == 1)
	{
		std::vector<unsigned char> serializedCommitment(33);
		secp256k1_pedersen_commitment_serialize(m_pContext, &serializedCommitment[0], &commitment);

		return std::make_unique<Commitment>(Commitment(CBigInteger<33>(std::move(serializedCommitment))));
	}

	LoggerAPI::LogError("Secp256k1Wrapper::PedersenCommitSum - secp256k1_pedersen_commit_sum returned result: " + std::to_string(result));
	return std::unique_ptr<Commitment>(nullptr);
}

std::unique_ptr<BlindingFactor> Pedersen::PedersenBlindSum(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative) const
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

std::unique_ptr<SecretKey> Pedersen::BlindSwitch(const SecretKey& blindingFactor, const uint64_t amount) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<unsigned char> blindSwitch(32);
	const int result = secp256k1_blind_switch(m_pContext, blindSwitch.data(), blindingFactor.data(), amount, &secp256k1_generator_const_h, &secp256k1_generator_const_g, &GENERATOR_J_PUB);
	if (result == 1)
	{
		return std::make_unique<SecretKey>(SecretKey(CBigInteger<32>(std::move(blindSwitch))));
	}

	return std::unique_ptr<SecretKey>(nullptr);
}

std::vector<secp256k1_pedersen_commitment*> Pedersen::ConvertCommitments(const secp256k1_context& context, const std::vector<Commitment>& commitments)
{
	std::vector<secp256k1_pedersen_commitment*> convertedCommitments(commitments.size(), NULL);
	for (int i = 0; i < commitments.size(); i++)
	{
		const std::vector<unsigned char>& commitmentBytes = commitments[i].GetCommitmentBytes().GetData();

		secp256k1_pedersen_commitment* pCommitment = new secp256k1_pedersen_commitment();
		const int parsed = secp256k1_pedersen_commitment_parse(&context, pCommitment, &commitmentBytes[0]);
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

void Pedersen::CleanupCommitments(std::vector<secp256k1_pedersen_commitment*>& commitments)
{
	for (secp256k1_pedersen_commitment* pCommitment : commitments)
	{
		delete pCommitment;
	}

	commitments.clear();
}