#pragma once

// Forward Declarations
class ChainState;

class ChainResyncer
{
public:
	ChainResyncer(ChainState& chainState);

	bool ResyncChain();

private:
	ChainState& m_chainState;
};