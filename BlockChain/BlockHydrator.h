#pragma once

#include "ChainState.h"

#include <Core/CompactBlock.h>
#include <Core/FullBlock.h>
#include <Core/Transaction.h>
#include <TxPool/TransactionPool.h>
#include <memory>

class BlockHydrator
{
public:
	BlockHydrator(const ChainState& chainState, const ITransactionPool& transactionPool);

	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock) const;

private:
	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock, const std::vector<Transaction>& transactions) const;
	void PerformCutThrough(std::vector<TransactionInput>& inputs, std::vector<TransactionOutput>& outputs) const;

	const ChainState& m_chainState;
	const ITransactionPool& m_transactionPool;
};