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
		std::shared_ptr<KernelMMR> pKernelMMR,
		std::shared_ptr<OutputPMMR> pOutputPMMR,
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR,
		BlockHeaderPtr pBlockHeader
	);
	virtual ~TxHashSet() = default;

	inline const BlockHeaderPtr& GetBlockHeader() const { return m_pBlockHeader; }
	virtual BlockHeaderPtr GetFlushedBlockHeader() const noexcept override final { return m_pBlockHeaderBackup; }

	virtual bool IsUnspent(const OutputLocation& location) const override final;
	virtual bool IsValid(std::shared_ptr<const IBlockDB> pBlockDB, const Transaction& transaction) const override final;
	virtual std::unique_ptr<BlockSums> ValidateTxHashSet(const BlockHeader& header, const IBlockChainServer& blockChainServer, SyncStatus& syncStatus) override final;
	virtual bool ApplyBlock(std::shared_ptr<IBlockDB> pBlockDB, const FullBlock& block) override final;
	virtual bool ValidateRoots(const BlockHeader& blockHeader) const override final;
	virtual void SaveOutputPositions(std::shared_ptr<IBlockDB> pBlockDB, const BlockHeader& blockHeader, const uint64_t firstOutputIndex) override final;

	virtual std::vector<Hash> GetLastKernelHashes(const uint64_t numberOfKernels) const override final;
	virtual std::vector<Hash> GetLastOutputHashes(const uint64_t numberOfOutputs) const override final;
	virtual std::vector<Hash> GetLastRangeProofHashes(const uint64_t numberOfRangeProofs) const override final;
	virtual OutputRange GetOutputsByLeafIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t maxNumOutputs) const override final;
	virtual std::vector<OutputDTO> GetOutputsByMMRIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t lastIndex) const override final;

	virtual void Rewind(std::shared_ptr<const IBlockDB> pBlockDB, const BlockHeader& header) override final;
	virtual void Commit() override final;
	virtual void Rollback() noexcept override final;
	virtual void Compact() override final;

	std::shared_ptr<KernelMMR> GetKernelMMR() { return m_pKernelMMR; }
	std::shared_ptr<OutputPMMR> GetOutputPMMR() { return m_pOutputPMMR; }
	std::shared_ptr<RangeProofPMMR> GetRangeProofPMMR() { return m_pRangeProofPMMR; }

private:
	std::shared_ptr<KernelMMR> m_pKernelMMR;
	std::shared_ptr<OutputPMMR> m_pOutputPMMR;
	std::shared_ptr<RangeProofPMMR> m_pRangeProofPMMR;

	BlockHeaderPtr m_pBlockHeader;
	BlockHeaderPtr m_pBlockHeaderBackup;
};
