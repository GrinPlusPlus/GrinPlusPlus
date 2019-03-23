#include "Bulletproofs.h"
#include "Pedersen.h"
#include "secp256k1-zkp/include/secp256k1_bulletproofs.h"

#include <Common/Util/FunctionalUtil.h>
#include <Crypto/RandomNumberGenerator.h>

const uint64_t MAX_WIDTH = 1 << 20;
const size_t SCRATCH_SPACE_SIZE = 256 * MAX_WIDTH;
const size_t MAX_GENERATORS = 256;

Bulletproofs& Bulletproofs::GetInstance()
{
	static Bulletproofs instance;
	return instance;
}

Bulletproofs::Bulletproofs()
{
	m_pContext = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);
	m_pGenerators = secp256k1_bulletproof_generators_create(m_pContext, &secp256k1_generator_const_g, MAX_GENERATORS);
}

Bulletproofs::~Bulletproofs()
{
	secp256k1_bulletproof_generators_destroy(m_pContext, m_pGenerators);
	secp256k1_context_destroy(m_pContext);
}

bool Bulletproofs::VerifyBulletproofs(const std::vector<std::pair<Commitment, RangeProof>>& rangeProofs) const
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
	std::vector<secp256k1_pedersen_commitment*> commitmentPointers = Pedersen::ConvertCommitments(*m_pContext, commitments);

	std::vector<const unsigned char*> bulletproofPointers;
	bulletproofPointers.reserve(rangeProofs.size());
	for (const std::pair<Commitment, RangeProof>& rangeProof : rangeProofs)
	{
		bulletproofPointers.emplace_back(rangeProof.second.GetProofBytes().data());
	}

	secp256k1_scratch_space* pScratchSpace = secp256k1_scratch_space_create(m_pContext, SCRATCH_SPACE_SIZE);
	const int result = secp256k1_bulletproof_rangeproof_verify_multi(m_pContext, pScratchSpace, m_pGenerators, bulletproofPointers.data(), rangeProofs.size(), proofLength, NULL, commitmentPointers.data(), 1, numBits, valueGenerators.data(), NULL, NULL);
	secp256k1_scratch_space_destroy(pScratchSpace);

	Pedersen::CleanupCommitments(commitmentPointers);

	return result == 1;
}

std::unique_ptr<RangeProof> Bulletproofs::GenerateRangeProof(const uint64_t amount, const SecretKey& key, const SecretKey& nonce, const ProofMessage& proofMessage) const
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

	std::vector<const unsigned char*> blindingFactors({ key.data() });
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
		nonce.data(),
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

std::unique_ptr<RewoundProof> Bulletproofs::RewindProof(const Commitment& commitment, const RangeProof& rangeProof, const SecretKey& nonce) const
{
	std::shared_lock<std::shared_mutex> readLock(m_mutex);

	std::vector<secp256k1_pedersen_commitment*> commitmentPointers = Pedersen::ConvertCommitments(*m_pContext, std::vector<Commitment>({ commitment }));

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
			nonce.data(),
			NULL,
			0,
			message.data()
		);
		Pedersen::CleanupCommitments(commitmentPointers);

		if (result == 1)
		{
			return std::make_unique<RewoundProof>(RewoundProof(value, SecretKey(std::move(blindingFactorBytes)), ProofMessage(std::move(message))));
		}
	}

	return std::unique_ptr<RewoundProof>(nullptr);
}