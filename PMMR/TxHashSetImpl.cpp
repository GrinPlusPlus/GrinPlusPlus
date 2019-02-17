#include "TxHashSetImpl.h"
#include "TxHashSetValidator.h"
#include "Common/MMRUtil.h"

#include <HexUtil.h>
#include <FileUtil.h>
#include <StringUtil.h>
#include <BlockChainServer.h>
#include <Database/BlockDb.h>
#include <Infrastructure/Logger.h>
#include <async++.h>

TxHashSet::TxHashSet(IBlockDB& blockDB, KernelMMR* pKernelMMR, OutputPMMR* pOutputPMMR, RangeProofPMMR* pRangeProofPMMR, const BlockHeader& blockHeader)
	: m_blockDB(blockDB), m_pKernelMMR(pKernelMMR), m_pOutputPMMR(pOutputPMMR), m_pRangeProofPMMR(pRangeProofPMMR), m_blockHeader(blockHeader), m_blockHeaderBackup(blockHeader)
{

}

TxHashSet::~TxHashSet()
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	delete m_pKernelMMR;
	delete m_pOutputPMMR;
	delete m_pRangeProofPMMR;
}

bool TxHashSet::IsUnspent(const OutputLocation& location) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	return m_pOutputPMMR->IsUnspent(location.GetMMRIndex());
}

bool TxHashSet::IsValid(const Transaction& transaction) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

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
			LoggerAPI::LogDebug("TxHashSet::IsValid - Output " + HexUtil::ConvertToHex(commitment.GetCommitmentBytes().GetData(), false, false) + " not found at mmrIndex " + std::to_string(outputPosOpt.value().GetMMRIndex()));
			return false;
		}
	}

	return true;
}

std::unique_ptr<BlockSums> TxHashSet::ValidateTxHashSet(const BlockHeader& header, const IBlockChainServer& blockChainServer)
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	LoggerAPI::LogInfo("TxHashSet::ValidateTxHashSet - Validating TxHashSet for block " + HexUtil::ConvertHash(header.GetHash()));
	std::unique_ptr<BlockSums> pBlockSums = TxHashSetValidator(blockChainServer).Validate(*this, header);
	if (pBlockSums != nullptr)
	{
		LoggerAPI::LogInfo("TxHashSet::ValidateTxHashSet - Successfully validated TxHashSet.");
	}

	return pBlockSums;
}

bool TxHashSet::ApplyBlock(const FullBlock& block)
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

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

	m_blockHeader = block.GetBlockHeader();

	return true;
}

bool TxHashSet::ValidateRoots(const BlockHeader& blockHeader) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

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
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

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

std::vector<Hash> TxHashSet::GetLastKernelHashes(const uint64_t numberOfKernels) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	return m_pKernelMMR->GetLastLeafHashes(numberOfKernels);
}

std::vector<Hash> TxHashSet::GetLastOutputHashes(const uint64_t numberOfOutputs) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	return m_pOutputPMMR->GetLastLeafHashes(numberOfOutputs);
}

std::vector<Hash> TxHashSet::GetLastRangeProofHashes(const uint64_t numberOfRangeProofs) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	return m_pRangeProofPMMR->GetLastLeafHashes(numberOfRangeProofs);
}

OutputRange TxHashSet::GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	const uint64_t outputSize = m_pOutputPMMR->GetSize();
	
	uint64_t leafIndex = startIndex;
	std::vector<OutputDisplayInfo> outputs;
	outputs.reserve(maxNumOutputs);
	while (outputs.size() < maxNumOutputs)
	{
		const uint64_t mmrIndex = MMRUtil::GetPMMRIndex(leafIndex++);
		if (mmrIndex >= outputSize)
		{
			break;
		}

		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(mmrIndex);
		if (pOutput != nullptr)
		{
			std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetRangeProofAt(mmrIndex);
			std::optional<OutputLocation> locationOpt = m_blockDB.GetOutputPosition(pOutput->GetCommitment());
			if (pRangeProof == nullptr || !locationOpt.has_value() || locationOpt.value().GetMMRIndex() != mmrIndex)
			{
				return OutputRange(0, 0, std::vector<OutputDisplayInfo>());
			}

			outputs.emplace_back(OutputDisplayInfo(false, *pOutput, locationOpt.value(), *pRangeProof));
		}
	}

	const uint64_t maxLeafIndex = MMRUtil::GetNumLeaves(outputSize - 1);
	const uint64_t lastRetrievedIndex = outputs.empty() ? 0 : MMRUtil::GetNumLeaves(outputs.back().GetLocation().GetMMRIndex());

	return OutputRange(maxLeafIndex, lastRetrievedIndex, std::move(outputs));
}

bool TxHashSet::Rewind(const BlockHeader& header)
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	// TODO: Use block input bitmaps
	Roaring leavesToAdd;
	while (m_blockHeader != header)
	{
		std::unique_ptr<FullBlock> pBlock = m_blockDB.GetBlock(m_blockHeader.GetHash());
		if (pBlock != nullptr)
		{
			for (const TransactionInput& input : pBlock->GetTransactionBody().GetInputs())
			{
				std::optional<OutputLocation> locationOpt = m_blockDB.GetOutputPosition(input.GetCommitment());
				if (locationOpt.has_value())
				{
					leavesToAdd.add(locationOpt.value().GetMMRIndex());
				}
			}
		}

		m_blockHeader = *m_blockDB.GetBlockHeader(m_blockHeader.GetPreviousBlockHash());
	}

	m_pKernelMMR->Rewind(header.GetKernelMMRSize());
	m_pOutputPMMR->Rewind(header.GetOutputMMRSize(), leavesToAdd);
	m_pRangeProofPMMR->Rewind(header.GetOutputMMRSize(), leavesToAdd);

	return true;
}

bool TxHashSet::Commit()
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	async::task<bool> kernelTask = async::spawn([this] { return this->m_pKernelMMR->Flush(); });
	async::task<bool> outputTask = async::spawn([this] { return this->m_pOutputPMMR->Flush(); });
	async::task<bool> rangeProofTask = async::spawn([this] { return this->m_pRangeProofPMMR->Flush(); });

	const bool flushed = async::when_all(kernelTask, outputTask, rangeProofTask).then(
		[](std::tuple<async::task<bool>, async::task<bool>, async::task<bool>> results) -> bool {
		return std::get<0>(results).get() && std::get<1>(results).get() && std::get<2>(results).get();
	}).get();

	m_blockHeaderBackup = m_blockHeader;
	return true;
}

bool TxHashSet::Discard()
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	m_pKernelMMR->Discard();
	m_pOutputPMMR->Discard();
	m_pRangeProofPMMR->Discard();
	m_blockHeader = m_blockHeaderBackup;
	return true;
}

bool TxHashSet::Compact()
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	// TODO: Implement

	return true;
}