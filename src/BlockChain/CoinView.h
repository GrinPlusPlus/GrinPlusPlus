#pragma once

#include <BlockChain/ICoinView.h>

#include "ChainState.h"

class CoinView : public ICoinView
{
public:
    CoinView(const Locked<ChainState>& state)
        : m_state(state) { }

    EOutputFeatures GetOutputType(const Commitment& commitment) const final;

private:
    Locked<ChainState> m_state;
};