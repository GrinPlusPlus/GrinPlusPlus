#pragma once

#include "ChainState.h"

#include <Core/Models/CompactBlock.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/Transaction.h>
#include <TxPool/TransactionPool.h>
#include <memory>

class BlockHydrator
{
public:
	BlockHydrator(const ChainState& chainState, const ITransactionPool& transactionPool);

	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock) const;

private:
	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock, const std::vector<Transaction>& transactions) const;

	const ChainState& m_chainState;
	const ITransactionPool& m_transactionPool;
};