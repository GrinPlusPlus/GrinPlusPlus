#pragma once

#include "KernelMMR.h"
#include "OutputPMMR.h"
#include "RangeProofPMMR.h"

#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <shared_mutex>
#include <string>

// TODO: Implement an "UpdateContext" object that can be committed to.
class TxHashSet : public ITxHashSet
{
public:
	TxHashSet(IBlockDB& blockDB, KernelMMR* pKernelMMR, OutputPMMR* pOutputPMMR, RangeProofPMMR* pRangeProofPMMR, const BlockHeader& blockHeader);
	~TxHashSet();

	inline void ReadLock() { m_txHashSetMutex.lock_shared(); }
	inline void Unlock() { m_txHashSetMutex.unlock_shared(); }
	inline const BlockHeader& GetBlockHeader() const { return m_blockHeader; }
	inline const BlockHeader& GetFlushedBlockHeader() const { return m_blockHeaderBackup; }

	virtual bool IsUnspent(const OutputLocation& location) const override final;
	virtual bool IsValid(const Transaction& transaction) const override final;
	virtual std::unique_ptr<BlockSums> ValidateTxHashSet(const BlockHeader& header, const IBlockChainServer& blockChainServer, SyncStatus& syncStatus) override final;
	virtual bool ApplyBlock(const FullBlock& block) override final;
	virtual bool ValidateRoots(const BlockHeader& blockHeader) const override final;
	virtual bool SaveOutputPositions(const BlockHeader& blockHeader, const uint64_t firstOutputIndex) override final;

	virtual std::vector<Hash> GetLastKernelHashes(const uint64_t numberOfKernels) const override final;
	virtual std::vector<Hash> GetLastOutputHashes(const uint64_t numberOfOutputs) const override final;
	virtual std::vector<Hash> GetLastRangeProofHashes(const uint64_t numberOfRangeProofs) const override final;
	virtual OutputRange GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const override final;
	virtual std::vector<OutputDisplayInfo> GetOutputsByMMRIndex(const uint64_t startIndex, const uint64_t lastIndex) const override final;

	virtual bool Rewind(const BlockHeader& header) override final;
	virtual bool Commit() override final;
	virtual bool Discard() override final;
	virtual bool Compact() override final;

	KernelMMR* GetKernelMMR() { return m_pKernelMMR; }
	OutputPMMR* GetOutputPMMR() { return m_pOutputPMMR; }
	RangeProofPMMR* GetRangeProofPMMR() { return m_pRangeProofPMMR; }

private:
	IBlockDB& m_blockDB;

	KernelMMR* m_pKernelMMR;
	OutputPMMR* m_pOutputPMMR;
	RangeProofPMMR* m_pRangeProofPMMR;

	BlockHeader m_blockHeader;
	BlockHeader m_blockHeaderBackup;

	mutable std::shared_mutex m_txHashSetMutex;
};