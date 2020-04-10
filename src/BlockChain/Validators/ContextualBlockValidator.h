#pragma once

#include <Core/Models/FullBlock.h>
#include <Config/Config.h>
#include <Database/BlockDb.h>
#include <PMMR/TxHashSet.h>
#include <memory>

class ContextualBlockValidator
{
public:
	ContextualBlockValidator(
		const Config& config,
		const std::shared_ptr<const IBlockDB>& pBlockDB,
		const ITxHashSetConstPtr& pTxHashSet)
	: m_config(config), m_pBlockDB(pBlockDB), m_pTxHashSet(pTxHashSet) { }

	BlockSums ValidateBlock(const FullBlock& block) const;

private:
	const Config& m_config;
	std::shared_ptr<const IBlockDB> m_pBlockDB;
	ITxHashSetConstPtr m_pTxHashSet;
};