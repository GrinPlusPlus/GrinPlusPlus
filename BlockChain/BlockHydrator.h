#pragma once

#include "ChainState.h"
#include "TransactionPool.h"

#include <Core/CompactBlock.h>
#include <Core/FullBlock.h>
#include <Core/Transaction.h>
#include <memory>

class BlockHydrator
{
public:
	BlockHydrator(const ChainState& chainState, const TransactionPool& transactionPool);

	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock) const;

private:
	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock, const std::vector<Transaction>& transactions) const;

	const ChainState& m_chainState;
	const TransactionPool& m_transactionPool;
};