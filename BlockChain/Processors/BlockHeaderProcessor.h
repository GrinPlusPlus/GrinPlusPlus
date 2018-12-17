#pragma once

#include "../ChainState.h"

#include <BlockChainStatus.h>
#include <Core/BlockHeader.h>

class BlockHeaderProcessor
{
public:
	BlockHeaderProcessor(ChainState& chainState);

	EBlockChainStatus ProcessSyncHeaders(const std::vector<BlockHeader>& headers);
	EBlockChainStatus ProcessSingleHeader(const BlockHeader& header);

private:
	EBlockChainStatus ProcessChunkedSyncHeaders(const std::vector<BlockHeader>& headers);
	EBlockChainStatus AddSyncHeaders(LockedChainState& lockedState, const std::vector<BlockHeader>& headers) const;
	bool CheckAndAcceptSyncChain(LockedChainState& lockedState) const;

	ChainState& m_chainState;
};