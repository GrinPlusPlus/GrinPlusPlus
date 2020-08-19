#include "ContextualBlockValidator.h"
#include "BlockValidator.h"

#include <Core/Exceptions/BlockChainException.h>
#include <Core/Exceptions/BadDataException.h>
#include <Core/Validation/KernelSumValidator.h>
#include <Consensus/Common.h>

// Validates a block is self-consistent and validates the state (eg. MMRs).
BlockSums ContextualBlockValidator::ValidateBlock(const FullBlock& block) const
{
	BlockValidator::VerifySelfConsistent(block);

	// Verify coinbase maturity
	const uint64_t maximumBlockHeight = Consensus::GetMaxCoinbaseHeight(
		m_config.GetEnvironment().GetType(),
		block.GetHeight()
	);
	for (const TransactionInput& input : block.GetInputs())
	{
		if (input.IsCoinbase())
		{
			const std::unique_ptr<OutputLocation> pOutputLocation = m_pBlockDB->GetOutputPosition(input.GetCommitment());
			if (pOutputLocation == nullptr || pOutputLocation->GetBlockHeight() > maximumBlockHeight)
			{
				LOG_INFO_F("Coinbase not mature for block {}", block);
				throw BAD_DATA_EXCEPTION("Failed to validate coinbase maturity.");
			}
		}
	}

	if (!m_pTxHashSet->ValidateRoots(*block.GetHeader()))
	{
		LOG_ERROR_F("Failed to validate TxHashSet roots for block {}", block);
		throw BAD_DATA_EXCEPTION("Failed to validate TxHashSet roots.");
	}

	const Hash previousHash = block.GetPreviousHash();
	std::unique_ptr<BlockSums> pPreviousBlockSums = m_pBlockDB->GetBlockSums(previousHash);
	if (pPreviousBlockSums == nullptr)
	{
		LOG_WARNING_F("Failed to retrieve block sums for block {}", previousHash);
		throw BLOCK_CHAIN_EXCEPTION("Failed to retrieve block sums.");
	}

	return KernelSumValidator::ValidateKernelSums(
		block.GetTransactionBody(),
		0 - Consensus::REWARD,
		block.GetTotalKernelOffset(),
		std::make_optional(*pPreviousBlockSums)
	);
}