#pragma once

#include <Core/Models/BlockHeader.h>
#include <BlockChain/BlockChain.h>
#include <P2P/SyncStatus.h>
#include <Crypto/Models/Hash.h>

class BlockLocator
{
public:
	BlockLocator(const IBlockChain::Ptr& pBlockChain)
		: m_pBlockChain(pBlockChain) { }

	std::vector<Hash> GetLocators(const SyncStatus& syncStatus) const;
	std::vector<BlockHeaderPtr> LocateHeaders(const std::vector<Hash>& locatorHashes) const;

private:
	std::vector<uint64_t> GetLocatorHeights(const SyncStatus& syncStatus) const;
	BlockHeaderPtr FindCommonHeader(const std::vector<Hash>& locatorHashes) const;

	IBlockChain::Ptr m_pBlockChain;
};