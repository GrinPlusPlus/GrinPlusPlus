#pragma once

#include "ChainState.h"

class ChainResyncer
{
public:
	explicit ChainResyncer(const std::shared_ptr<Locked<ChainState>>& pChainState);

	void ResyncChain();

private:
	void CleanDatabase(const std::shared_ptr<IBlockDB>& pBlockDB);

	std::shared_ptr<Locked<ChainState>> m_pChainState;
};