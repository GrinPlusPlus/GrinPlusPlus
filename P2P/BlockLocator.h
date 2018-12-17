#pragma once

#include <Core/BlockHeader.h>
#include <BlockChainServer.h>

class BlockLocator
{
public:
	BlockLocator(IBlockChainServer& blockChainServer);

	std::vector<CBigInteger<32>> GetLocators() const;
	std::vector<BlockHeader> LocateHeaders(const std::vector<CBigInteger<32>>& locatorHashes) const;

private:
	std::vector<uint64_t> GetLocatorHeights() const;
	std::unique_ptr<BlockHeader> FindCommonHeader(const std::vector<CBigInteger<32>>& locatorHashes) const;

	IBlockChainServer& m_blockChainServer;
};