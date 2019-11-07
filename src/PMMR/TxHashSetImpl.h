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
		const BlockHeader& blockHeader
	);
	virtual ~TxHashSet() = default;

	inline void ReadLock() { m_txHashSetMutex.lock_shared(); }
	inline void Unlock() { m_txHashSetMutex.unlock_shared(); }
	inline const BlockHeader& GetBlockHeader() const { return m_blockHeader; }
	virtual const BlockHeader& GetFlushedBlockHeader() const override final { return m_blockHeaderBackup; }

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
	virtual std::vector<OutputDisplayInfo> GetOutputsByMMRIndex(std::shared_ptr<const IBlockDB> pBlockDB, const uint64_t startIndex, const uint64_t lastIndex) const override final;

	virtual bool Rewind(std::shared_ptr<const IBlockDB> pBlockDB, const BlockHeader& header) override final;
	virtual void Commit() override final;
	virtual void Rollback() override final;
	virtual bool Compact() override final;

	std::shared_ptr<KernelMMR> GetKernelMMR() { return m_pKernelMMR; }
	std::shared_ptr<OutputPMMR> GetOutputPMMR() { return m_pOutputPMMR; }
	std::shared_ptr<RangeProofPMMR> GetRangeProofPMMR() { return m_pRangeProofPMMR; }

private:
	std::shared_ptr<Locked<IBlockDB>> m_pBlockDB;

	std::shared_ptr<KernelMMR> m_pKernelMMR;
	std::shared_ptr<OutputPMMR> m_pOutputPMMR;
	std::shared_ptr<RangeProofPMMR> m_pRangeProofPMMR;

	BlockHeader m_blockHeader;
	BlockHeader m_blockHeaderBackup;

	mutable std::shared_mutex m_txHashSetMutex;
};
