#pragma once

#include "KernelMMR.h"
#include "OutputPMMR.h"
#include "RangeProofPMMR.h"

#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <shared_mutex>
#include <string>

class TxHashSet : public ITxHashSet
{
public:
	TxHashSet(
		const Config& config,
		std::shared_ptr<KernelMMR> pKernelMMR,
		std::shared_ptr<OutputPMMR> pOutputPMMR,
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR,
		BlockHeaderPtr pBlockHeader
	);
	virtual ~TxHashSet() = default;

	BlockHeaderPtr GetFlushedBlockHeader() const noexcept final { return m_pBlockHeaderBackup; }

	bool IsValid(std::shared_ptr<const IBlockDB> pBlockDB, const Transaction& transaction) const final;
	std::unique_ptr<BlockSums> ValidateTxHashSet(const BlockHeader& header, const IBlockChain& blockChain, SyncStatus& syncStatus) final;
	bool ApplyBlock(std::shared_ptr<IBlockDB> pBlockDB, const FullBlock& block) final;
	bool ValidateRoots(const BlockHeader& blockHeader) const final;
	TxHashSetRoots GetRoots(const std::shared_ptr<const IBlockDB>& pBlockDB, const TransactionBody& body) final;
	void SaveOutputPositions(const Chain::CPtr& pChain, std::shared_ptr<IBlockDB> pBlockDB) const final;

	std::vector<Hash> GetLastKernelHashes(const uint64_t numberOfKernels) const final;
	std::vector<Hash> GetLastOutputHashes(const uint64_t numberOfOutputs) const final;
	std::vector<Hash> GetLastRangeProofHashes(const uint64_t numberOfRangeProofs) const final;
	OutputRange GetOutputsByLeafIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t maxNumOutputs) const final;
	std::vector<OutputDTO> GetOutputsByMMRIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t lastIndex) const final;
	OutputDTO GetOutput(const OutputLocation& location) const final;

	void Rewind(std::shared_ptr<IBlockDB> pBlockDB, const BlockHeader& header) final;
	void Commit() final;
	void Rollback() noexcept final;
	void Compact() final;

	std::shared_ptr<KernelMMR> GetKernelMMR() { return m_pKernelMMR; }
	std::shared_ptr<OutputPMMR> GetOutputPMMR() { return m_pOutputPMMR; }
	std::shared_ptr<RangeProofPMMR> GetRangeProofPMMR() { return m_pRangeProofPMMR; }

private:
	const Config& m_config;
	std::shared_ptr<KernelMMR> m_pKernelMMR;
	std::shared_ptr<OutputPMMR> m_pOutputPMMR;
	std::shared_ptr<RangeProofPMMR> m_pRangeProofPMMR;

	BlockHeaderPtr m_pBlockHeader;
	BlockHeaderPtr m_pBlockHeaderBackup;
};
