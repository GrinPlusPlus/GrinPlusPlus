#include "TxHashSetImpl.h"
#include "TxHashSetValidator.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Common/Util/ThreadUtil.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <BlockChain/BlockChain.h>
#include <Database/BlockDb.h>
#include <Common/Logger.h>
#include <P2P/SyncStatus.h>
#include <thread>

TxHashSet::TxHashSet(
	const Config& config,
	std::shared_ptr<KernelMMR> pKernelMMR,
	std::shared_ptr<OutputPMMR> pOutputPMMR,
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR,
	BlockHeaderPtr pBlockHeader)
	: m_config(config),
	m_pKernelMMR(pKernelMMR),
	m_pOutputPMMR(pOutputPMMR),
	m_pRangeProofPMMR(pRangeProofPMMR),
	m_pBlockHeader(pBlockHeader),
	m_pBlockHeaderBackup(pBlockHeader)
{

}

bool TxHashSet::IsValid(std::shared_ptr<const IBlockDB> pBlockDB, const Transaction& transaction) const
{
	// Validate inputs
	const uint64_t next_height = m_pBlockHeader->GetHeight() + 1;
	const uint64_t maximumBlockHeight = Consensus::GetMaxCoinbaseHeight(next_height);
	for (const TransactionInput& input : transaction.GetInputs())
	{
		const Commitment& commitment = input.GetCommitment();
		std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(commitment);
		if (pOutputPosition == nullptr) {
			return false;
		}

		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetMMRIndex());
		if (pOutput == nullptr || pOutput->GetCommitment() != commitment) {
			LOG_DEBUG_F("Output ({}) not found at mmrIndex ({})",  commitment, pOutputPosition->GetMMRIndex());
			return false;
		}

		if (pOutput->IsCoinbase()) {
			if (pOutputPosition->GetBlockHeight() > maximumBlockHeight) {
				LOG_INFO_F("Coinbase {} not mature", input.GetCommitment());
				return false;
			}
		}
	}

	// Validate outputs
	for (const TransactionOutput& output : transaction.GetOutputs())
	{
		std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(output.GetCommitment());
		if (pOutputPosition != nullptr) {
			std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetMMRIndex());
			if (pOutput != nullptr && pOutput->GetCommitment() == output.GetCommitment())
			{
				return false;
			}
		}
	}

	return true;
}

std::unique_ptr<BlockSums> TxHashSet::ValidateTxHashSet(const BlockHeader& header, const IBlockChain& blockChain, SyncStatus& syncStatus)
{
	std::unique_ptr<BlockSums> pBlockSums = nullptr;

	try
	{
		LOG_INFO("Validating TxHashSet for block " + header.GetHash().ToHex());
		pBlockSums = TxHashSetValidator(blockChain).Validate(*this, header, syncStatus);
		if (pBlockSums != nullptr)
		{
			LOG_INFO("Successfully validated TxHashSet");
		}
	}
	catch (...)
	{
		LOG_ERROR("Exception thrown while processing TxHashSet");
	}

	return pBlockSums;
}

bool TxHashSet::ApplyBlock(std::shared_ptr<IBlockDB> pBlockDB, const FullBlock& block)
{
	// Validate inputs
	const uint64_t maximumBlockHeight = Consensus::GetMaxCoinbaseHeight(block.GetHeight());

	Roaring blockInputBitmap;

	// Prune inputs
	std::vector<SpentOutput> spentPositions;
	spentPositions.reserve(block.GetInputs().size());

	for (const TransactionInput& input : block.GetInputs())
	{
		const Commitment& commitment = input.GetCommitment();
		std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(commitment);
		if (pOutputPosition == nullptr) {
			LOG_WARNING_F("Output position not found for commitment {} in block {}", commitment, block);
			return false;
		}

		auto pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetMMRIndex());
		assert(pOutput != nullptr);

		if (pOutput->IsCoinbase()) {
			if (pOutputPosition->GetBlockHeight() > maximumBlockHeight) {
				LOG_WARNING_F("Coinbase {} not mature", input.GetCommitment());
				return false;
			}
		}

		spentPositions.push_back(SpentOutput(commitment, *pOutputPosition));

		const uint64_t mmrIndex = pOutputPosition->GetMMRIndex();
		m_pOutputPMMR->Remove(mmrIndex);
		m_pRangeProofPMMR->Remove(mmrIndex);
	}

	pBlockDB->AddSpentPositions(block.GetHash(), spentPositions);

	// Append new outputs
	for (const TransactionOutput& output : block.GetOutputs())
	{
		std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(output.GetCommitment());
		if (pOutputPosition != nullptr && m_pOutputPMMR->IsUnpruned(pOutputPosition->GetMMRIndex())) {
			LOG_ERROR_F("Output {} already exists at position {} and height {}",
				output,
				pOutputPosition->GetMMRIndex(),
				pOutputPosition->GetBlockHeight()
			);
			return false;
		}

		const uint64_t mmrIndex = m_pOutputPMMR->GetSize();
		const uint64_t blockHeight = block.GetHeight();

		m_pOutputPMMR->Append(OutputIdentifier::FromOutput(output));
		m_pRangeProofPMMR->Append(output.GetRangeProof());

		pBlockDB->AddOutputPosition(output.GetCommitment(), OutputLocation(mmrIndex, blockHeight));
	}

	pBlockDB->RemoveOutputPositions(block.GetInputCommitments());

	// Append new kernels
	for (const TransactionKernel& kernel : block.GetKernels())
	{
		m_pKernelMMR->ApplyKernel(kernel);
	}

	m_pBlockHeader = block.GetHeader();

	return true;
}

bool TxHashSet::ValidateRoots(const BlockHeader& blockHeader) const
{
	if (m_pKernelMMR->Root(blockHeader.GetKernelMMRSize()) != blockHeader.GetKernelRoot())
	{
		LOG_ERROR_F("Kernel root not matching for header ({})", blockHeader);
		return false;
	}

	Hash outputRoot = m_pOutputPMMR->Root(blockHeader.GetOutputMMRSize());
	if (blockHeader.GetVersion() < 3)
	{
		if (outputRoot != blockHeader.GetOutputRoot())
		{
			LOG_ERROR_F("Output root not matching for header ({})", blockHeader);
			return false;
		}
	}
	else
	{
		Hash UBMT = m_pOutputPMMR->UBMTRoot(MMRUtil::GetNumLeaves(blockHeader.GetOutputMMRSize() - 1));
		Hash merged = MMRHashUtil::HashParentWithIndex(outputRoot, UBMT, blockHeader.GetOutputMMRSize());
		if (merged != blockHeader.GetOutputRoot())
		{
			LOG_ERROR_F("Output root not matching for header ({}). Output: {}, UBMT: {}", blockHeader, outputRoot, UBMT);
			return false;
		}
	}

	if (m_pRangeProofPMMR->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetRangeProofRoot())
	{
		LOG_ERROR_F("RangeProof root not matching for header ({})", blockHeader);
		return false;
	}

	return true;
}

TxHashSetRoots TxHashSet::GetRoots(const std::shared_ptr<const IBlockDB>& pBlockDB, const TransactionBody& body)
{
	for (const auto& kernel : body.GetKernels())
	{
		m_pKernelMMR->ApplyKernel(kernel);
	}

	for (const auto& input : body.GetInputs())
	{
		const auto pOutputPosition = pBlockDB->GetOutputPosition(input.GetCommitment());
		if (pOutputPosition == nullptr)
		{
			throw std::exception();
		}

		m_pOutputPMMR->Remove(pOutputPosition->GetMMRIndex());
		m_pRangeProofPMMR->Remove(pOutputPosition->GetMMRIndex());
	}

	for (const auto& output : body.GetOutputs())
	{
		m_pOutputPMMR->Append(OutputIdentifier::FromOutput(output));
		m_pRangeProofPMMR->Append(output.GetRangeProof());
	}

	const uint64_t numKernels = MMRUtil::GetLeafIndex(m_pBlockHeader->GetKernelMMRSize()) + body.GetKernels().size();
	const uint64_t kernelSize = MMRUtil::GetPMMRIndex(numKernels);
	const auto kernelRoot = m_pKernelMMR->Root(kernelSize);

	const uint64_t numOutputs = MMRUtil::GetLeafIndex(m_pBlockHeader->GetOutputMMRSize()) + body.GetOutputs().size();
	const uint64_t outputSize = MMRUtil::GetPMMRIndex(numOutputs);
	const auto outputRoot = m_pOutputPMMR->Root(outputSize);
	const auto rangeProofRoot = m_pRangeProofPMMR->Root(outputSize);

	return TxHashSetRoots(
		{ kernelRoot, kernelSize },
		{ outputRoot, outputSize },
		{ rangeProofRoot, outputSize }
	);
}

void TxHashSet::SaveOutputPositions(const Chain::CPtr& pChain, std::shared_ptr<IBlockDB> pBlockDB) const
{
	uint64_t firstOutput = 0;
	for (uint64_t height = 0; height <= m_pBlockHeader->GetHeight(); height++)
	{
		auto pIndex = pChain->GetByHeight(height);
		if (pIndex != nullptr)
		{
			auto pHeader = pBlockDB->GetBlockHeader(pIndex->GetHash());
			if (pHeader != nullptr)
			{
				const uint64_t size = pHeader->GetOutputMMRSize();
				for (uint64_t mmrIndex = firstOutput; mmrIndex < size; mmrIndex++)
				{
					std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(mmrIndex);
					if (pOutput != nullptr)
					{
						OutputLocation location(mmrIndex, pHeader->GetHeight());
						pBlockDB->AddOutputPosition(pOutput->GetCommitment(), location);
					}
				}

				firstOutput = pHeader->GetOutputMMRSize();
			}
		}
	}
}

std::vector<Hash> TxHashSet::GetLastKernelHashes(const uint64_t numberOfKernels) const
{
	return m_pKernelMMR->GetLastLeafHashes(numberOfKernels);
}

std::vector<Hash> TxHashSet::GetLastOutputHashes(const uint64_t numberOfOutputs) const
{
	return m_pOutputPMMR->GetLastLeafHashes(numberOfOutputs);
}

std::vector<Hash> TxHashSet::GetLastRangeProofHashes(const uint64_t numberOfRangeProofs) const
{
	return m_pRangeProofPMMR->GetLastLeafHashes(numberOfRangeProofs);
}

OutputRange TxHashSet::GetOutputsByLeafIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t maxNumOutputs) const
{
	const uint64_t outputSize = m_pOutputPMMR->GetSize();
	
	uint64_t leafIndex = startIndex;
	std::vector<OutputDTO> outputs;
	outputs.reserve(maxNumOutputs);
	while (outputs.size() < maxNumOutputs)
	{
		const uint64_t mmrIndex = MMRUtil::GetPMMRIndex(leafIndex++);
		if (mmrIndex >= outputSize)
		{
			break;
		}

		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(mmrIndex);
		if (pOutput != nullptr)
		{
			std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(mmrIndex);
			std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(pOutput->GetCommitment());
			if (pRangeProof == nullptr || pOutputPosition == nullptr || pOutputPosition->GetMMRIndex() != mmrIndex)
			{
				throw TXHASHSET_EXCEPTION_F("Failed to build OutputDTO at index {}", mmrIndex);
			}

			outputs.emplace_back(OutputDTO(false, *pOutput, *pOutputPosition, *pRangeProof));
		}
	}

	const uint64_t maxLeafIndex = MMRUtil::GetNumLeaves(outputSize - 1);
	const uint64_t lastRetrievedIndex = outputs.empty() ? 0 : MMRUtil::GetNumLeaves(outputs.back().GetLocation().GetMMRIndex());

	return OutputRange(maxLeafIndex, lastRetrievedIndex, std::move(outputs));
}

std::vector<OutputDTO> TxHashSet::GetOutputsByMMRIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t lastIndex) const
{
	std::vector<OutputDTO> outputs;
	uint64_t mmrIndex = startIndex;
	while (mmrIndex <= lastIndex)
	{
		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(mmrIndex);
		if (pOutput != nullptr)
		{
			std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(mmrIndex);
			std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(pOutput->GetCommitment());

			outputs.emplace_back(OutputDTO(false, *pOutput, *pOutputPosition, *pRangeProof));
		}

		++mmrIndex;
	}

	return outputs;
}

OutputDTO TxHashSet::GetOutput(const OutputLocation& location) const
{
	std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(location.GetMMRIndex());
	std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(location.GetMMRIndex());
	if (pOutput == nullptr || pRangeProof == nullptr) {
		throw TXHASHSET_EXCEPTION_F("Output not found at mmr index {}", location.GetMMRIndex());
	}

	return OutputDTO(false, *pOutput, location, *pRangeProof);
}

void TxHashSet::Rewind(std::shared_ptr<IBlockDB> pBlockDB, const BlockHeader& header)
{
	std::vector<uint64_t> leavesToAdd;
	while (*m_pBlockHeader != header)
	{
		auto pBlock = pBlockDB->GetBlock(m_pBlockHeader->GetHash());
		if (pBlock == nullptr)
		{
			throw TXHASHSET_EXCEPTION_F("Block not found for {}", *m_pBlockHeader);
		}

		std::unordered_map<Commitment, OutputLocation> spentOutputs = pBlockDB->GetSpentPositions(m_pBlockHeader->GetHash());

		pBlockDB->RemoveOutputPositions(pBlock->GetOutputCommitments());

		for (const auto& input : pBlock->GetInputs())
		{
			auto iter = spentOutputs.find(input.GetCommitment());
			if (iter == spentOutputs.end())
			{
				throw TXHASHSET_EXCEPTION_F("Spent output not found for {}", input.GetCommitment());
			}

			pBlockDB->AddOutputPosition(input.GetCommitment(), iter->second);
			leavesToAdd.push_back(MMRUtil::GetLeafIndex(iter->second.GetMMRIndex()));
		}

		m_pBlockHeader = pBlockDB->GetBlockHeader(m_pBlockHeader->GetPreviousHash());
	}

	m_pKernelMMR->Rewind(header.GetKernelMMRSize());
	m_pOutputPMMR->Rewind(header.GetOutputMMRSize(), leavesToAdd);
	m_pRangeProofPMMR->Rewind(header.GetOutputMMRSize(), leavesToAdd);
}

void TxHashSet::Commit()
{
	std::vector<std::thread> threads;
	threads.emplace_back(std::thread([this] { this->m_pKernelMMR->Commit(); }));
	threads.emplace_back(std::thread([this] { this->m_pOutputPMMR->Commit(); }));
	threads.emplace_back(std::thread([this] { this->m_pRangeProofPMMR->Commit(); }));
	ThreadUtil::JoinAll(threads);

	m_pBlockHeaderBackup = m_pBlockHeader;
}

void TxHashSet::Rollback() noexcept
{
	m_pKernelMMR->Rollback();
	m_pOutputPMMR->Rollback();
	m_pRangeProofPMMR->Rollback();
	m_pBlockHeader = m_pBlockHeaderBackup;
}

void TxHashSet::Compact()
{
	// TODO: Implement
}