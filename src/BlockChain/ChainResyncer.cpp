#include "ChainResyncer.h"
#include "ChainState.h"

ChainResyncer::ChainResyncer(std::shared_ptr<Locked<ChainState>> pChainState)
	: m_pChainState(pChainState)
{

}

bool ChainResyncer::ResyncChain()
{
	auto pLockedState = m_pChainState->BatchWrite();

	std::shared_ptr<Chain> pConfirmedChain = pLockedState->GetChainStore()->GetConfirmedChain();
	pConfirmedChain->Rewind(0);

	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();
	
	pLockedState->GetHeaderMMR()->Rewind(1);
	for (uint64_t i = 1; i <= pCandidateChain->GetTip()->GetHeight(); i++)
	{
		std::shared_ptr<const BlockIndex> pIndex = pCandidateChain->GetByHeight(i);
		const Hash hash = pIndex->GetHash();

		std::unique_ptr<BlockHeader> pHeader = pLockedState->GetBlockDB()->GetBlockHeader(hash);
		if (pHeader == nullptr)
		{
			pCandidateChain->Rewind(i - 1);
			break;
		}

		pLockedState->GetHeaderMMR()->AddHeader(*pHeader);
	}

	std::shared_ptr<Chain> pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	pSyncChain->Rewind(pCandidateChain->GetTip()->GetHeight());

	pLockedState->Commit();

	return true;
}