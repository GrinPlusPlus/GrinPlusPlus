#include "CoinView.h"

#include <Core/Exceptions/BlockChainException.h>

EOutputFeatures CoinView::GetOutputType(const Commitment& commitment) const
{
    auto pState = m_state.Read();
    auto pBlockDB = pState->GetBlockDB();

    auto pLocation = pBlockDB->GetOutputPosition(commitment);
    if (pLocation == nullptr) {
        throw BLOCK_CHAIN_EXCEPTION_F("Output {} not found.", commitment);
    }

    auto pTxHashSet = pState->GetTxHashSetManager()->GetTxHashSet();
    OutputDTO output = pTxHashSet->GetOutput(*pLocation);

    return output.GetIdentifier().GetFeatures();
}