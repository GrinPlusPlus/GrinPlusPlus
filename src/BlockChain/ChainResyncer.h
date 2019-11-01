#pragma once

#include "ChainState.h"

class ChainResyncer
{
public:
	ChainResyncer(std::shared_ptr<Locked<ChainState>> pChainState);

	bool ResyncChain();

private:
	std::shared_ptr<Locked<ChainState>> m_pChainState;
};