#pragma once

#include "ChainState.h"

class ChainResyncer
{
public:
	explicit ChainResyncer(std::shared_ptr<Locked<ChainState>> pChainState);

	bool ResyncChain();

private:
	std::shared_ptr<Locked<ChainState>> m_pChainState;
};