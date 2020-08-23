#include "ChainResyncer.h"

#include <Common/Logger.h>
#include <Database/BlockDb.h>

ChainResyncer::ChainResyncer(const std::shared_ptr<Locked<ChainState>>& pChainState)
	: m_pChainState(pChainState) { }

void ChainResyncer::ResyncChain()
{
	LOG_WARNING("Chain resync requested!");

	auto pLockedState = m_pChainState->BatchWrite();

	auto pBlockDB = pLockedState->GetBlockDB();
	auto pHeaderMMR = pLockedState->GetHeaderMMR();
	auto pConfirmedChain = pLockedState->GetChainStore()->GetConfirmedChain();
	auto pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();

	CleanDatabase(pBlockDB);

	pConfirmedChain->Rewind(0);

	pHeaderMMR->Rewind(1);
	auto pPrevHeader = pLockedState->GetBlockHeaderByHeight(0, EChainType::CANDIDATE);

	for (uint64_t i = 1; i <= pCandidateChain->GetHeight(); i++)
	{
		auto pIndex = pCandidateChain->GetByHeight(i);
		auto pHeader = pBlockDB->GetBlockHeader(pIndex->GetHash());
		if (pHeader == nullptr || pHeader->GetPreviousHash() != pPrevHeader->GetHash())
		{
			pCandidateChain->Rewind(i - 1);
			break;
		}

		pPrevHeader = pHeader;
		pHeaderMMR->AddHeader(*pHeader);
	}

	pLockedState->Commit();

	LOG_WARNING("Chain resync initiated!");
}

void ChainResyncer::CleanDatabase(const std::shared_ptr<IBlockDB>& pBlockDB)
{
	pBlockDB->ClearBlocks();
	pBlockDB->ClearBlockSums();
	pBlockDB->ClearOutputPositions();
	pBlockDB->ClearSpentPositions();
}