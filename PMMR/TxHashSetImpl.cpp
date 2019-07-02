#include "TxHashSetImpl.h"
#include "TxHashSetValidator.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/BlockDb.h>
#include <Infrastructure/Logger.h>
#include <P2P/SyncStatus.h>
#include <thread>

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

	// Validate inputs
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
			LoggerAPI::LogDebug("TxHashSet::IsValid - Output " + commitment.ToHex() + " not found at mmrIndex " + std::to_string(outputPosOpt.value().GetMMRIndex()));
			return false;
		}
	}

	// Validate outputs
	for (const TransactionOutput& output : transaction.GetBody().GetOutputs())
	{
		std::optional<OutputLocation> outputPosOpt = m_blockDB.GetOutputPosition(output.GetCommitment());
		if (outputPosOpt.has_value())
		{
			std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(outputPosOpt.value().GetMMRIndex());
			if (pOutput != nullptr && pOutput->GetCommitment() == output.GetCommitment())
			{
				return false;
			}
		}
	}

	return true;
}

std::unique_ptr<BlockSums> TxHashSet::ValidateTxHashSet(const BlockHeader& header, const IBlockChainServer& blockChainServer, SyncStatus& syncStatus)
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	std::unique_ptr<BlockSums> pBlockSums = nullptr;

	try
	{
		LoggerAPI::LogInfo("TxHashSet::ValidateTxHashSet - Validating TxHashSet for block " + HexUtil::ConvertHash(header.GetHash()));
		pBlockSums = TxHashSetValidator(blockChainServer).Validate(*this, header, syncStatus);
		if (pBlockSums != nullptr)
		{
			LoggerAPI::LogInfo("TxHashSet::ValidateTxHashSet - Successfully validated TxHashSet.");
		}
	}
	catch (std::exception& e)
	{
		LoggerAPI::LogError("TxHashSet::ValidateTxHashSet - Exception thrown while processing TxHashSet.");
	}

	return pBlockSums;
}

bool TxHashSet::ApplyBlock(const FullBlock& block)
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	Roaring blockInputBitmap;

	// Prune inputs
	for (const TransactionInput& input : block.GetTransactionBody().GetInputs())
	{
		const Commitment& commitment = input.GetCommitment();
		std::optional<OutputLocation> outputPosOpt = m_blockDB.GetOutputPosition(commitment);
		if (!outputPosOpt.has_value())
		{
			LoggerAPI::LogWarning("TxHashSet::ApplyBlock - Output position not found for commitment: " + commitment.ToHex() + " in block: " + std::to_string(block.GetBlockHeader().GetHeight()));
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

		blockInputBitmap.add(mmrIndex + 1);
	}

	m_blockDB.AddBlockInputBitmap(block.GetBlockHeader().GetHash(), blockInputBitmap);

	// Append new outputs
	for (const TransactionOutput& output : block.GetTransactionBody().GetOutputs())
	{
		std::optional<OutputLocation> outputPosOpt = m_blockDB.GetOutputPosition(output.GetCommitment());
		if (outputPosOpt.has_value())
		{
			if (outputPosOpt.value().GetMMRIndex() < m_blockHeader.GetOutputMMRSize())
			{
				std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(outputPosOpt.value().GetMMRIndex());
				if (pOutput != nullptr && pOutput->GetCommitment() == output.GetCommitment())
				{
					return false;
				}
			}
		}

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

std::vector<OutputDisplayInfo> TxHashSet::GetOutputsByMMRIndex(const uint64_t startIndex, const uint64_t lastIndex) const
{
	std::shared_lock<std::shared_mutex> readLock(m_txHashSetMutex);

	std::vector<OutputDisplayInfo> outputs;
	uint64_t mmrIndex = startIndex;
	while (mmrIndex <= lastIndex)
	{
		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetOutputAt(mmrIndex);
		if (pOutput != nullptr)
		{
			std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetRangeProofAt(mmrIndex);
			std::optional<OutputLocation> locationOpt = m_blockDB.GetOutputPosition(pOutput->GetCommitment());

			outputs.emplace_back(OutputDisplayInfo(false, *pOutput, locationOpt.value(), *pRangeProof));
		}

		++mmrIndex;
	}

	return outputs;
}

bool TxHashSet::Rewind(const BlockHeader& header)
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	Roaring leavesToAdd;
	while (m_blockHeader != header)
	{
		std::optional<Roaring> blockInputBitmapOpt = m_blockDB.GetBlockInputBitmap(m_blockHeader.GetHash());
		if (blockInputBitmapOpt.has_value())
		{
			leavesToAdd |= blockInputBitmapOpt.value();
		}
		else
		{
			return false;
		}

		m_blockHeader = *m_blockDB.GetBlockHeader(m_blockHeader.GetPreviousBlockHash());
	}

	m_pKernelMMR->Rewind(header.GetKernelMMRSize());
	m_pOutputPMMR->Rewind(header.GetOutputMMRSize(), std::make_optional<Roaring>(leavesToAdd));
	m_pRangeProofPMMR->Rewind(header.GetOutputMMRSize(), std::make_optional<Roaring>(leavesToAdd));

	return true;
}

bool TxHashSet::Commit()
{
	std::unique_lock<std::shared_mutex> writeLock(m_txHashSetMutex);

	std::vector<std::thread> threads;
	threads.emplace_back(std::thread([this] { this->m_pKernelMMR->Flush(); }));
	threads.emplace_back(std::thread([this] { this->m_pOutputPMMR->Flush(); }));
	threads.emplace_back(std::thread([this] { this->m_pRangeProofPMMR->Flush(); }));
	for (auto& thread : threads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

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