#include "ChainResyncer.h"

#include <Infrastructure/Logger.h>

ChainResyncer::ChainResyncer(const std::shared_ptr<Locked<ChainState>>& pChainState)
	: m_pChainState(pChainState) { }

void ChainResyncer::ResyncChain()
{
	LOG_WARNING("Chain resync requested!");

	auto pLockedState = m_pChainState->BatchWrite();

	auto pBlockDB = pLockedState->GetBlockDB();
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	//auto pSyncChain = pLockedState->GetChainStore()->GetSyncChain();
	auto pConfirmedChain = pLockedState->GetChainStore()->GetConfirmedChain();
	auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	CleanDatabase(pBlockDB);

	pConfirmedChain->Rewind(0);

	pHeaderMMR->Rewind(1);
	for (uint64_t i = 1; i <= pCandidateChain->GetHeight(); i++)
	{
		auto pIndex = pCandidateChain->GetByHeight(i);
		auto pHeader = pBlockDB->GetBlockHeader(pIndex->GetHash());
		if (pHeader == nullptr)
		{
			pCandidateChain->Rewind(i - 1);
			break;
		}

		pHeaderMMR->AddHeader(*pHeader);
	}

	//pSyncChain->Rewind(pCandidateChain->GetTip()->GetHeight());

	pLockedState->Commit();

	LOG_WARNING("Chain resync initiated!");
}

void ChainResyncer::CleanDatabase(const std::shared_ptr<IBlockDB>& pBlockDB)
{
	pBlockDB->ClearBlockSums();
	pBlockDB->ClearOutputPositions();
	pBlockDB->ClearSpentPositions();
}