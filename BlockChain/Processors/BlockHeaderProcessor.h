#pragma once

#include "../ChainState.h"

#include <Config/Config.h>
#include <BlockChain/BlockChainStatus.h>
#include <Core/Models/BlockHeader.h>

class BlockHeaderProcessor
{
public:
	BlockHeaderProcessor(const Config& config, ChainState& chainState);

	EBlockChainStatus ProcessSyncHeaders(const std::vector<BlockHeader>& headers);
	EBlockChainStatus ProcessSingleHeader(const BlockHeader& header);

private:
	EBlockChainStatus ProcessChunkedSyncHeaders(LockedChainState& lockedState, const std::vector<const BlockHeader*>& headers);
	EBlockChainStatus AddSyncHeaders(LockedChainState& lockedState, const std::vector<const BlockHeader*>& headers) const;
	bool CheckAndAcceptSyncChain(LockedChainState& lockedState) const;

	const Config& m_config;
	ChainState& m_chainState;
};