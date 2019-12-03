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
	explicit BlockHydrator(std::shared_ptr<const ITransactionPool> pTransactionPool);

	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock) const;

private:
	std::unique_ptr<FullBlock> Hydrate(const CompactBlock& compactBlock, const std::vector<TransactionPtr>& transactions) const;

	std::shared_ptr<const ITransactionPool> m_pTransactionPool;
};