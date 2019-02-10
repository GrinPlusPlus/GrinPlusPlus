#include "TxHashSetImpl.h"
#include "TxHashSetValidator.h"

#include <HexUtil.h>
#include <FileUtil.h>
#include <StringUtil.h>
#include <BlockChainServer.h>
#include <Database/BlockDb.h>
#include <Infrastructure/Logger.h>

TxHashSet::TxHashSet(IBlockDB& blockDB, KernelMMR* pKernelMMR, OutputPMMR* pOutputPMMR, RangeProofPMMR* pRangeProofPMMR)
	: m_blockDB(blockDB), m_pKernelMMR(pKernelMMR), m_pOutputPMMR(pOutputPMMR), m_pRangeProofPMMR(pRangeProofPMMR)
{

}

TxHashSet::~TxHashSet()
{
	delete m_pKernelMMR;
	delete m_pOutputPMMR;
	delete m_pRangeProofPMMR;
}

bool TxHashSet::IsUnspent(const OutputIdentifier& output) const
{
	const std::optional<OutputLocation> outputLocationOpt = m_blockDB.GetOutputPosition(output.GetCommitment());
	if (outputLocationOpt.has_value())
	{
		// TODO: I believe this should call GetOutputAt. GetHashAt returns spent hashes, as long as they aren't pruned.
		std::unique_ptr<Hash> pMMRHash = m_pOutputPMMR->GetHashAt(outputLocationOpt.value().GetMMRIndex());
		if (pMMRHash != nullptr)
		{
			Serializer serializer;
			output.Serialize(serializer);
			const Hash outputHash = Crypto::Blake2b(serializer.GetBytes());

			if (outputHash == *pMMRHash)
			{
				return true;
			}
		}
	}

	return false;
}

bool TxHashSet::IsValid(const Transaction& transaction) const
{
	for (const TransactionInput& input : transaction.GetBody().GetInputs())
	{
		const Commitment& commitment = input.GetCommitment();
		std::optional<OutputLocation> outputPosOpt = m_blockDB.GetOutputPosition(commitment);
		if (!outputPosOpt.has_value())
		{
			return false;
		}

		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(outputPosOpt.value().GetMMRIndex());
		if (pOutput == nullptr || pOutput->GetCommitment() != commitment || pOutput->GetFeatures() != input.GetFeatures())
		{
			LoggerAPI::LogWarning("TxHashSet::IsValid - Transaction invalid: " + HexUtil::ConvertHash(transaction.GetHash()));
			return false;
		}
	}

	return true;
}

bool TxHashSet::ValidateTxHashSet(const BlockHeader& header, const IBlockChainServer& blockChainServer, Commitment& outputSumOut, Commitment& kernelSumOut)
{
	LoggerAPI::LogInfo("TxHashSet::ValidateTxHashSet - Validating TxHashSet for block " + HexUtil::ConvertHash(header.GetHash()));
	const TxHashSetValidationResult result = TxHashSetValidator(blockChainServer).Validate(*this, header, outputSumOut, kernelSumOut);
	if (result.Successful())
	{
		LoggerAPI::LogInfo("TxHashSet::ValidateTxHashSet - Successfully validated TxHashSet.");
		outputSumOut = result.GetOutputSum();
		kernelSumOut = result.GetKernelSum();
		return true;
	}

	return false;
}

bool TxHashSet::ApplyBlock(const FullBlock& block)
{
	// Prune inputs
	for (const TransactionInput& input : block.GetTransactionBody().GetInputs())
	{
		const Commitment& commitment = input.GetCommitment();
		std::optional<OutputLocation> outputPosOpt = m_blockDB.GetOutputPosition(commitment);
		if (!outputPosOpt.has_value())
		{
			LoggerAPI::LogWarning("TxHashSet::ApplyBlock - Output position not found for commitment: " + HexUtil::ConvertToHex(commitment.GetCommitmentBytes().GetData(), false, false) + " in block: " + std::to_string(block.GetBlockHeader().GetHeight()));
			return false;
		}

		const uint64_t mmrIndex = outputPosOpt.value().GetMMRIndex();
		if (!m_pOutputPMMR->Remove(mmrIndex))
		{
			LoggerAPI::LogWarning("TxHashSet::ApplyBlock - Failed to remove output at position:" + std::to_string(mmrIndex));
			return false;
		}

		if (!m_pRangeProofPMMR->Remove(mmrIndex))
		{
			LoggerAPI::LogWarning("TxHashSet::ApplyBlock - Failed to remove rangeproof at position:" + std::to_string(mmrIndex));
			return false;
		}
	}

	// Append new outputs
	for (const TransactionOutput& output : block.GetTransactionBody().GetOutputs())
	{
		if (!m_pOutputPMMR->Append(OutputIdentifier::FromOutput(output), block.GetBlockHeader().GetHeight()))
		{
			return false;
		}

		if (!m_pRangeProofPMMR->Append(output.GetRangeProof()))
		{
			return false;
		}
	}

	// Append new kernels
	for (const TransactionKernel& kernel : block.GetTransactionBody().GetKernels())
	{
		m_pKernelMMR->ApplyKernel(kernel);
	}

	return true;
}

bool TxHashSet::ValidateRoots(const BlockHeader& blockHeader) const
{
	if (m_pKernelMMR->Root(blockHeader.GetKernelMMRSize()) != blockHeader.GetKernelRoot())
	{
		LoggerAPI::LogError("TxHashSet::ValidateRoots - Kernel root not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	if (m_pOutputPMMR->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetOutputRoot())
	{
		LoggerAPI::LogError("TxHashSet::ValidateRoots - Output root not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	if (m_pRangeProofPMMR->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetRangeProofRoot())
	{
		LoggerAPI::LogError("TxHashSet::ValidateRoots - RangeProof root not matching for header " + HexUtil::ConvertHash(blockHeader.GetHash()));
		return false;
	}

	return true;
}

bool TxHashSet::SaveOutputPositions(const BlockHeader& blockHeader, const uint64_t firstOutputIndex)
{
	const uint64_t size = blockHeader.GetOutputMMRSize();
	for (uint64_t mmrIndex = firstOutputIndex; mmrIndex < size; mmrIndex++)
	{
		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(mmrIndex);
		if (pOutput != nullptr)
		{
			m_blockDB.AddOutputPosition(pOutput->GetCommitment(), OutputLocation(mmrIndex, blockHeader.GetHeight()));
		}
	}

	return true;
}

bool TxHashSet::Snapshot(const BlockHeader& header)
{
	return true;
}

bool TxHashSet::Rewind(const BlockHeader& header)
{
	m_pKernelMMR->Rewind(header.GetKernelMMRSize());
	m_pOutputPMMR->Rewind(header.GetOutputMMRSize());
	m_pRangeProofPMMR->Rewind(header.GetOutputMMRSize());
	return true;
}

bool TxHashSet::Commit()
{
	m_pKernelMMR->Flush();
	m_pOutputPMMR->Flush();
	m_pRangeProofPMMR->Flush();
	return true;
}

bool TxHashSet::Discard()
{
	m_pKernelMMR->Discard();
	m_pOutputPMMR->Discard();
	m_pRangeProofPMMR->Discard();
	return true;
}

bool TxHashSet::Compact()
{
	return true;
}