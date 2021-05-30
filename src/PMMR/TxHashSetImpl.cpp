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

		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetLeafIndex());
		if (pOutput == nullptr || pOutput->GetCommitment() != commitment) {
			LOG_DEBUG_F("Output ({}) not found at mmrIndex ({})",  commitment, pOutputPosition->GetLeafIndex());
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
			std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetLeafIndex());
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

	for (const TransactionInput& input : block.GetInputs()) {
		const Commitment& commitment = input.GetCommitment();
		std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(commitment);
		if (pOutputPosition == nullptr) {
			LOG_WARNING_F("Output position not found for commitment {} in block {}", commitment, block);
			return false;
		}

		auto pOutput = m_pOutputPMMR->GetAt(pOutputPosition->GetLeafIndex());
		assert(pOutput != nullptr);

		if (pOutput->IsCoinbase() && pOutputPosition->GetBlockHeight() > maximumBlockHeight) {
			LOG_WARNING_F("Coinbase {} not mature", input.GetCommitment());
			return false;
		}

		spentPositions.push_back(SpentOutput(commitment, *pOutputPosition));

		m_pOutputPMMR->Remove(pOutputPosition->GetLeafIndex());
		m_pRangeProofPMMR->Remove(pOutputPosition->GetLeafIndex());
	}

	pBlockDB->AddSpentPositions(block.GetHash(), spentPositions);

	// Append new outputs
	for (const TransactionOutput& output : block.GetOutputs()) {
		std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(output.GetCommitment());
		if (pOutputPosition != nullptr && m_pOutputPMMR->IsUnpruned(pOutputPosition->GetLeafIndex())) {
			LOG_ERROR_F("Output {} already exists at {} and height {}",
				output,
				pOutputPosition->GetLeafIndex(),
				pOutputPosition->GetBlockHeight()
			);
			return false;
		}

		OutputLocation location(LeafIndex::AtPos(m_pOutputPMMR->GetSize()), block.GetHeight());

		m_pOutputPMMR->Append(OutputIdentifier::FromOutput(output));
		m_pRangeProofPMMR->Append(output.GetRangeProof());

		pBlockDB->AddOutputPosition(output.GetCommitment(), location);
	}

	pBlockDB->RemoveOutputPositions(block.GetInputCommitments());

	// Append new kernels
	for (const TransactionKernel& kernel : block.GetKernels()) {
		m_pKernelMMR->ApplyKernel(kernel);
	}

	m_pBlockHeader = block.GetHeader();

	return true;
}

bool TxHashSet::ValidateRoots(const BlockHeader& blockHeader) const
{
    if (m_pKernelMMR->Root(blockHeader.GetKernelMMRSize()) != blockHeader.GetKernelRoot()) {
        LOG_ERROR_F("Kernel root not matching for header ({})", blockHeader);
        return false;
    }

    Hash outputRoot = m_pOutputPMMR->Root(blockHeader.GetOutputMMRSize());
    if (blockHeader.GetVersion() < 3) {
        if (outputRoot != blockHeader.GetOutputRoot()) {
            LOG_ERROR_F("Output root not matching for header ({})", blockHeader);
            return false;
        }
    } else {
        Hash UBMT = m_pOutputPMMR->UBMTRoot(blockHeader.GetNumOutputs());
        Hash merged = MMRHashUtil::HashParentWithIndex(outputRoot, UBMT, blockHeader.GetOutputMMRSize());
        if (merged != blockHeader.GetOutputRoot()) {
            LOG_ERROR_F("Output root not matching for header ({}). Output: {}, UBMT: {}", blockHeader, outputRoot, UBMT);
            return false;
        }
    }

    if (m_pRangeProofPMMR->Root(blockHeader.GetOutputMMRSize()) != blockHeader.GetRangeProofRoot()) {
        LOG_ERROR_F("RangeProof root not matching for header ({})", blockHeader);
        return false;
    }

	return true;
}

TxHashSetRoots TxHashSet::GetRoots(const std::shared_ptr<const IBlockDB>& pBlockDB, const TransactionBody& body)
{
	for (const auto& kernel : body.GetKernels()) {
		m_pKernelMMR->ApplyKernel(kernel);
	}

	for (const auto& input : body.GetInputs()) {
		const auto pOutputPosition = pBlockDB->GetOutputPosition(input.GetCommitment());
		if (pOutputPosition == nullptr) {
			throw std::runtime_error(StringUtil::Format("Output position not found for input {}", input.GetCommitment()));
		}

		m_pOutputPMMR->Remove(pOutputPosition->GetLeafIndex());
		m_pRangeProofPMMR->Remove(pOutputPosition->GetLeafIndex());
	}

	for (const auto& output : body.GetOutputs()) {
		m_pOutputPMMR->Append(OutputIdentifier::FromOutput(output));
		m_pRangeProofPMMR->Append(output.GetRangeProof());
	}

	const uint64_t num_kernels = m_pBlockHeader->GetNumKernels() + body.GetKernels().size();
	const uint64_t kernel_size = LeafIndex::At(num_kernels).GetPosition();
	const auto kernel_root = m_pKernelMMR->Root(kernel_size);

	const uint64_t num_outputs = m_pBlockHeader->GetNumOutputs() + body.GetOutputs().size();
	const uint64_t output_size = LeafIndex::At(num_outputs).GetPosition();
	const auto output_root = m_pOutputPMMR->Root(output_size);
	const auto proof_root = m_pRangeProofPMMR->Root(output_size);

	return TxHashSetRoots(
		{ kernel_root, kernel_size },
		{ output_root, output_size },
		{ proof_root, output_size }
	);
}

void TxHashSet::SaveOutputPositions(const Chain::CPtr& pChain, std::shared_ptr<IBlockDB> pBlockDB) const
{
	uint64_t firstOutput = 0;
	LeafIndex leaf_idx = LeafIndex::At(0);
	for (uint64_t height = 0; height <= m_pBlockHeader->GetHeight(); height++) {
		auto pIndex = pChain->GetByHeight(height);
		if (pIndex == nullptr) {
			continue;
		}

		auto pHeader = pBlockDB->GetBlockHeader(pIndex->GetHash());
		if (pHeader == nullptr) {
			continue;
		}

		while (leaf_idx < pHeader->GetNumOutputs()) {
			std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(leaf_idx);
			if (pOutput != nullptr) {
				OutputLocation location(leaf_idx, pHeader->GetHeight());
				pBlockDB->AddOutputPosition(pOutput->GetCommitment(), location);
			}

			++leaf_idx;
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
	
	LeafIndex leaf_idx = LeafIndex::At(startIndex);
	std::vector<OutputDTO> outputs;
	outputs.reserve(maxNumOutputs);
	while (outputs.size() < maxNumOutputs)
	{
		if (leaf_idx.GetPosition() >= outputSize) {
			break;
		}

		std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(leaf_idx);
		if (pOutput != nullptr) {
			std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(leaf_idx);
			std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(pOutput->GetCommitment());
			if (pRangeProof == nullptr || pOutputPosition == nullptr || leaf_idx != pOutputPosition->GetLeafIndex()) {
				throw TXHASHSET_EXCEPTION_F("Failed to build OutputDTO at {}", leaf_idx);
			}

			outputs.emplace_back(OutputDTO(false, *pOutput, *pOutputPosition, *pRangeProof));
		}

		++leaf_idx;
	}

	const uint64_t maxLeafIndex = LeafIndex::AtPos(outputSize);
	const uint64_t lastRetrievedIndex = outputs.empty() ? 0 : outputs.back().GetLeafIndex().Get();

	return OutputRange(maxLeafIndex > 0 ? maxLeafIndex - 1 : 0, lastRetrievedIndex, std::move(outputs));
}

std::vector<OutputDTO> TxHashSet::GetOutputsByMMRIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t lastIndex) const
{
	std::vector<OutputDTO> outputs;
	Index mmr_idx = Index::At(startIndex);
	while (mmr_idx <= lastIndex) {
		if (mmr_idx.IsLeaf()) {
			LeafIndex leaf_idx = LeafIndex::From(mmr_idx);
			std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(leaf_idx);
			if (pOutput != nullptr) {
				std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(leaf_idx);
				std::unique_ptr<OutputLocation> pOutputPosition = pBlockDB->GetOutputPosition(pOutput->GetCommitment());

				outputs.emplace_back(OutputDTO(false, *pOutput, *pOutputPosition, *pRangeProof));
			}
		}

		++mmr_idx;
	}

	return outputs;
}

OutputDTO TxHashSet::GetOutput(const OutputLocation& location) const
{
	std::unique_ptr<OutputIdentifier> pOutput = m_pOutputPMMR->GetAt(location.GetLeafIndex());
	std::unique_ptr<RangeProof> pRangeProof = m_pRangeProofPMMR->GetAt(location.GetLeafIndex());
	if (pOutput == nullptr || pRangeProof == nullptr) {
		throw TXHASHSET_EXCEPTION_F("Output not found at {}", location.GetLeafIndex());
	}

	return OutputDTO(false, *pOutput, location, *pRangeProof);
}

void TxHashSet::Rewind(std::shared_ptr<IBlockDB> pBlockDB, const BlockHeader& header)
{
	std::vector<uint64_t> leavesToAdd;
	while (*m_pBlockHeader != header) {
		auto pBlock = pBlockDB->GetBlock(m_pBlockHeader->GetHash());
		if (pBlock == nullptr) {
			throw TXHASHSET_EXCEPTION_F("Block not found for {}", *m_pBlockHeader);
		}

		std::unordered_map<Commitment, OutputLocation> spentOutputs = pBlockDB->GetSpentPositions(m_pBlockHeader->GetHash());

		pBlockDB->RemoveOutputPositions(pBlock->GetOutputCommitments());

		for (const auto& input : pBlock->GetInputs()) {
			auto iter = spentOutputs.find(input.GetCommitment());
			if (iter == spentOutputs.end()) {
				throw TXHASHSET_EXCEPTION_F("Spent output not found for {}", input.GetCommitment());
			}

			pBlockDB->AddOutputPosition(input.GetCommitment(), iter->second);
			leavesToAdd.push_back(iter->second.GetLeafIndex().Get());
		}

		m_pBlockHeader = pBlockDB->GetBlockHeader(m_pBlockHeader->GetPreviousHash());
	}

	m_pKernelMMR->Rewind(header.GetNumKernels());
	m_pOutputPMMR->Rewind(header.GetNumOutputs(), leavesToAdd);
	m_pRangeProofPMMR->Rewind(header.GetNumOutputs(), leavesToAdd);
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