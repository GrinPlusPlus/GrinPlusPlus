#pragma once

#include <Core/Models/BlockHeader.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/SyncStatus.h>

class BlockLocator
{
public:
	BlockLocator(IBlockChainServerPtr pBlockChainServer);

	std::vector<CBigInteger<32>> GetLocators(const SyncStatus& syncStatus) const;
	std::vector<BlockHeader> LocateHeaders(const std::vector<CBigInteger<32>>& locatorHashes) const;

private:
	std::vector<uint64_t> GetLocatorHeights(const SyncStatus& syncStatus) const;
	std::unique_ptr<BlockHeader> FindCommonHeader(const std::vector<CBigInteger<32>>& locatorHashes) const;

	IBlockChainServerPtr m_pBlockChainServer;
};