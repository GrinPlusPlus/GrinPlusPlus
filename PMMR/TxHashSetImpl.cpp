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
	const std::optional<uint64_t> mmrIndex = m_blockDB.GetOutputPosition(output.GetCommitment());
	if (mmrIndex.has_value())
	{
		std::unique_ptr<Hash> pMMRHash = m_pOutputPMMR->GetHashAt(mmrIndex.value());
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
		std::optional<uint64_t> outputPosOpt = m_blockDB.GetOutputPosition(commitment);
		if (!outputPosOpt.has_value())
		{
			return false;
		}

		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(outputPosOpt.value());
		if (pOutput == nullptr || pOutput->GetCommitment() != commitment || pOutput->GetFeatures() != input.GetFeatures())
		{
			LoggerAPI::LogWarning("TxHashSet::IsValid - Transaction invalid: " + HexUtil::ConvertHash(transaction.GetHash()));
			return false;
		}
	}

	return true;
}

bool TxHashSet::Validate(const BlockHeader& header, const IBlockChainServer& blockChainServer, Commitment& outputSumOut, Commitment& kernelSumOut)
{
	LoggerAPI::LogInfo("TxHashSet::Validate - Validating TxHashSet for block " + HexUtil::ConvertHash(header.GetHash()));
	const TxHashSetValidationResult result = TxHashSetValidator(blockChainServer).Validate(*this, header, outputSumOut, kernelSumOut);
	if (result.Successful())
	{
		LoggerAPI::LogInfo("TxHashSet::Validate - Successfully validated TxHashSet.");
		outputSumOut = result.GetOutputSum();
		kernelSumOut = result.GetKernelSum();
		return true;
	}

	return false;
}

bool TxHashSet::ApplyBlock(const FullBlock& block)
{
	for (const TransactionInput& input : block.GetTransactionBody().GetInputs())
	{
		const Commitment& commitment = input.GetCommitment();
		std::optional<uint64_t> outputPosOpt = m_blockDB.GetOutputPosition(commitment);
		if (!outputPosOpt.has_value())
		{
			return false;
		}

		if (!m_pOutputPMMR->SpendOutput(outputPosOpt.value()))
		{
			return false;
		}

		// TODO: Prune RangeProof
	}

	for (const TransactionOutput& output : block.GetTransactionBody().GetOutputs())
	{
		if (!m_pOutputPMMR->Append(OutputIdentifier::FromOutput(output)))
		{
			return false;
		}

		// TODO: Append RangeProof
	}

	for (const TransactionKernel& kernel : block.GetTransactionBody().GetKernels())
	{
		m_pKernelMMR->ApplyKernel(kernel);
	}

	return true;
}

// TODO: Also store block heights?
bool TxHashSet::SaveOutputPositions()
{
	const uint64_t size = m_pOutputPMMR->GetSize();
	for (uint64_t mmrIndex = 0; mmrIndex < size; mmrIndex++)
	{
		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(mmrIndex);
		if (pOutput != nullptr)
		{
			m_blockDB.AddOutputPosition(pOutput->GetCommitment(), mmrIndex);
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