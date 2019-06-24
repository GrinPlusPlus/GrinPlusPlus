#include "ChainResyncer.h"
#include "ChainState.h"

ChainResyncer::ChainResyncer(ChainState& chainState)
	: m_chainState(chainState)
{

}

bool ChainResyncer::ResyncChain()
{
	LockedChainState locked = m_chainState.GetLocked();

	Chain& confirmedChain = locked.m_chainStore.GetConfirmedChain();
	confirmedChain.Rewind(0);

	Chain& candidateChain = locked.m_chainStore.GetCandidateChain();
	
	locked.m_headerMMR.Rewind(1);
	for (uint64_t i = 1; i <= candidateChain.GetTip()->GetHeight(); i++)
	{
		BlockIndex* pIndex = candidateChain.GetByHeight(i);
		const Hash& hash = pIndex->GetHash();

		std::unique_ptr<BlockHeader> pHeader = locked.m_blockStore.GetBlockHeaderByHash(hash);
		if (pHeader == nullptr)
		{
			candidateChain.Rewind(i - 1);
			break;
		}

		locked.m_headerMMR.AddHeader(*pHeader);
	}

	locked.m_headerMMR.Commit();

	Chain& syncChain = locked.m_chainStore.GetSyncChain();
	syncChain.Rewind(candidateChain.GetTip()->GetHeight());

	confirmedChain.Flush();
	candidateChain.Flush();
	syncChain.Flush();

	return true;
}