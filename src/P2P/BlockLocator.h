#pragma once

#include <Core/Models/BlockHeader.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/SyncStatus.h>
#include <Crypto/Hash.h>

class BlockLocator
{
public:
	BlockLocator(IBlockChainServerPtr pBlockChainServer);

	std::vector<Hash> GetLocators(const SyncStatus& syncStatus) const;
	std::vector<BlockHeaderPtr> LocateHeaders(const std::vector<Hash>& locatorHashes) const;

private:
	std::vector<uint64_t> GetLocatorHeights(const SyncStatus& syncStatus) const;
	BlockHeaderPtr FindCommonHeader(const std::vector<Hash>& locatorHashes) const;

	IBlockChainServerPtr m_pBlockChainServer;
};