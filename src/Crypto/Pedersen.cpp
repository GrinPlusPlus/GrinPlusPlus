#include "Pedersen.h"
#include "SwitchGeneratorPoint.h"

#include <secp256k1-zkp/secp256k1_commitment.h>
#include <Core/Exceptions/CryptoException.h>
#include <Common/Logger.h>

static Pedersen instance;

Pedersen& Pedersen::GetInstance()
{
	//static Pedersen instance;
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

Commitment Pedersen::PedersenCommit(const uint64_t value, const BlindingFactor& blindingFactor) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pedersen_commitment commitment;
	const int result = secp256k1_pedersen_commit(m_pContext, &commitment, &blindingFactor.GetBytes()[0], value, &secp256k1_generator_const_h, &secp256k1_generator_const_g);
	if (result == 1)
	{
		Commitment commitment_out;
		secp256k1_pedersen_commitment_serialize(m_pContext, commitment_out.data(), &commitment);

		return commitment_out;
	}

	LOG_ERROR_F("Failed to create commitment. Result: {}, Value: {}", result, value);
	throw CryptoException("Failed to create commitment.");
}

Commitment Pedersen::PedersenCommitSum(const std::vector<Commitment>& positive, const std::vector<Commitment>& negative) const
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

	if (result != 1)
	{
		LOG_ERROR_F("secp256k1_pedersen_commit_sum returned result: {}", result);
		throw CryptoException("secp256k1_pedersen_commit_sum error");
	}


	std::vector<unsigned char> serializedCommitment(33);
	const int serializeResult = secp256k1_pedersen_commitment_serialize(m_pContext, &serializedCommitment[0], &commitment);
	if (serializeResult != 1)
	{
		LOG_ERROR_F("secp256k1_pedersen_commitment_serialize returned result: {}", serializeResult);
		throw CryptoException("secp256k1_pedersen_commitment_serialize error");
	}

	return Commitment(CBigInteger<33>(std::move(serializedCommitment)));
}

BlindingFactor Pedersen::PedersenBlindSum(const std::vector<BlindingFactor>& positive, const std::vector<BlindingFactor>& negative) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<const unsigned char*> blindingFactors;
	for (const BlindingFactor& positiveFactor : positive)
	{
		blindingFactors.push_back(positiveFactor.data());
	}

	for (const BlindingFactor& negativeFactor : negative)
	{
		blindingFactors.push_back(negativeFactor.data());
	}

	CBigInteger<32> blindingFactorBytes;
	const int result = secp256k1_pedersen_blind_sum(
		m_pContext,
		blindingFactorBytes.data(),
		blindingFactors.data(),
		blindingFactors.size(),
		positive.size()
	);

	if (result == 1)
	{
		return BlindingFactor(std::move(blindingFactorBytes));
	}

	LOG_ERROR_F("secp256k1_pedersen_blind_sum returned result: {}", result);
	throw CryptoException("secp256k1_pedersen_blind_sum error");
}

SecretKey Pedersen::BlindSwitch(const SecretKey& blindingFactor, const uint64_t amount) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<unsigned char> blindSwitch(32);
	const int result = secp256k1_blind_switch(
		m_pContext,
		blindSwitch.data(),
		blindingFactor.data(),
		amount,
		&secp256k1_generator_const_h,
		&secp256k1_generator_const_g,
		&GENERATOR_J_PUB
	);
	if (result == 1)
	{
		return SecretKey(CBigInteger<32>(std::move(blindSwitch)));
	}

	throw CryptoException("secp256k1_blind_switch failed with error: " + std::to_string(result));
}

Commitment Pedersen::ToCommitment(const PublicKey& publicKey) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	secp256k1_pubkey secp_pubkey;
	const int pubKeyParsed = secp256k1_ec_pubkey_parse(m_pContext, &secp_pubkey, publicKey.data(), publicKey.size());
	if (pubKeyParsed != 1) {
		throw CryptoException("secp256k1_ec_pubkey_parse failed");
	}

	secp256k1_pedersen_commitment secp_commitment;
	const int converted = secp256k1_pubkey_to_pedersen_commitment(m_pContext, &secp_commitment, &secp_pubkey);
	if (converted != 1) {
		throw CryptoException("secp256k1_pubkey_to_pedersen_commitment failed");
	}

	Commitment commitment;
	const int serialized = secp256k1_pedersen_commitment_serialize(m_pContext, commitment.data(), &secp_commitment);
	if (serialized != 1) {
		throw CryptoException("secp256k1_pedersen_commitment_serialize failed");
	}

	return commitment;

}

std::vector<secp256k1_pedersen_commitment*> Pedersen::ConvertCommitments(const secp256k1_context& context, const std::vector<Commitment>& commitments)
{
	std::vector<secp256k1_pedersen_commitment*> convertedCommitments(commitments.size(), NULL);
	for (int i = 0; i < commitments.size(); i++)
	{
		secp256k1_pedersen_commitment* pCommitment = new secp256k1_pedersen_commitment();
		const int parsed_result = secp256k1_pedersen_commitment_parse(&context, pCommitment, commitments[i].data());
		convertedCommitments[i] = pCommitment;

		if (parsed_result != 1) {
			CleanupCommitments(convertedCommitments);
			LOG_ERROR_F("secp256k1_pedersen_commitment_parse failed with error {} for commitment {}", parsed_result, commitments[i]);
			throw CRYPTO_EXCEPTION_F("secp256k1_pedersen_commitment_parse failed with error: {}", parsed_result);
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