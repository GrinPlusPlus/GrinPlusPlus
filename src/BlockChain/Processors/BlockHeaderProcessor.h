#pragma once

#include "../ChainState.h"

#include <Config/Config.h>
#include <BlockChain/BlockChainStatus.h>
#include <Core/Models/BlockHeader.h>

class BlockHeaderProcessor
{
public:
	BlockHeaderProcessor(const Config& config, std::shared_ptr<Locked<ChainState>> pChainState);

	EBlockChainStatus ProcessSyncHeaders(const std::vector<BlockHeader>& headers);
	EBlockChainStatus ProcessSingleHeader(const BlockHeader& header);

private:
	EBlockChainStatus ProcessChunkedSyncHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers);
	void ValidateHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers);
	void AddSyncHeaders(Writer<ChainState> pLockedState, const std::vector<std::shared_ptr<const BlockHeader>>& headers) const;
	bool CheckAndAcceptSyncChain(Writer<ChainState> pLockedState) const;

	const Config& m_config;
	std::shared_ptr<Locked<ChainState>> m_pChainState;
};