#pragma once

#include "KernelMMR.h"
#include "OutputPMMR.h"
#include "RangeProofPMMR.h"

#include <TxHashSet.h>
#include <Config/Config.h>
#include <string>

class TxHashSet : public ITxHashSet
{
public:
	TxHashSet(IBlockDB& blockDB, KernelMMR* pKernelMMR, OutputPMMR* pOutputPMMR, RangeProofPMMR* pRangeProofPMMR);
	~TxHashSet();

	virtual bool IsUnspent(const OutputIdentifier& output) const override final;
	virtual bool Validate(const BlockHeader& header, const IBlockChainServer& blockChainServer, Commitment& outputSumOut, Commitment& kernelSumOut) override final;
	virtual bool ApplyBlock(const FullBlock& block) override final;
	virtual bool SaveOutputPositions() override final;

	virtual bool Snapshot(const BlockHeader& header) override final;
	virtual bool Rewind(const BlockHeader& header) override final;
	virtual bool Compact() override final;

	KernelMMR* GetKernelMMR() { return m_pKernelMMR; }
	OutputPMMR* GetOutputPMMR() { return m_pOutputPMMR; }
	RangeProofPMMR* GetRangeProofPMMR() { return m_pRangeProofPMMR; }

private:
	IBlockDB& m_blockDB;

	KernelMMR* m_pKernelMMR;
	OutputPMMR* m_pOutputPMMR;
	RangeProofPMMR* m_pRangeProofPMMR;
};