#include "TxHashSetProcessor.h"

#include <Infrastructure/Logger.h>
#include <Database/BlockDb.h>
#include <Core/Models/BlockHeader.h>
#include <BlockChain/BlockChainServer.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/HexUtil.h>

TxHashSetProcessor::TxHashSetProcessor(const Config& config, IBlockChainServer& blockChainServer, ChainState& chainState, IBlockDB& blockDB)
	: m_config(config), m_blockChainServer(blockChainServer), m_chainState(chainState), m_blockDB(blockDB)
{

}

bool TxHashSetProcessor::ProcessTxHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus)
{
	std::unique_ptr<BlockHeader> pHeader = m_chainState.GetBlockHeaderByHash(blockHash);
	if (pHeader == nullptr)
	{
		LOG_ERROR_F("Header not found for hash %s.", HexUtil::ConvertHash(blockHash).c_str());
		return false;
	}

	// 1. Close Existing TxHashSet
	m_chainState.GetLocked().m_txHashSetManager.Close();

	// 2. Load and Extract TxHashSet Zip
	ITxHashSet* pTxHashSet = TxHashSetManager::LoadFromZip(m_config, m_blockDB, path, *pHeader);
	if (pTxHashSet == nullptr)
	{
		LOG_ERROR("Failed to load " + path);
		return false;
	}

	// 3. Validate entire TxHashSet
	std::unique_ptr<BlockSums> pBlockSums = pTxHashSet->ValidateTxHashSet(*pHeader, m_blockChainServer, syncStatus);
	if (pBlockSums == nullptr)
	{
		LOG_ERROR_F("Validation of %s failed.", path.c_str());
		TxHashSetManager::DestroyTxHashSet(pTxHashSet);
		return false;
	}

	// 4. Add BlockSums to DB
	m_blockDB.AddBlockSums(pHeader->GetHash(), *pBlockSums);

	// 5. Add Output positions to DB
	{
		LOG_DEBUG("Saving output positions.");
		LockedChainState lockedState = m_chainState.GetLocked();
		Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();

		uint64_t firstOutput = 0;
		for (uint64_t i = 0; i <= pHeader->GetHeight(); i++)
		{
			BlockIndex* pIndex = candidateChain.GetByHeight(i);
			std::unique_ptr<BlockHeader> pNextHeader = lockedState.m_blockStore.GetBlockHeaderByHash(pIndex->GetHash());
			if (pNextHeader != nullptr)
			{
				pTxHashSet->SaveOutputPositions(*pNextHeader, firstOutput);
				firstOutput = pNextHeader->GetOutputMMRSize();
			}
		}
	}

	// 6. Store TxHashSet
	LOG_DEBUG("Using TxHashSet.");
	LockedChainState lockedState = m_chainState.GetLocked();
	lockedState.m_txHashSetManager.SetTxHashSet(pTxHashSet);

	// 7. Update confirmed chain
	LOG_DEBUG("Updating confirmed chain.");
	if (!UpdateConfirmedChain(lockedState, *pHeader))
	{
		LOG_ERROR_F("Failed to update confirmed chain for %s.", path.c_str());
		lockedState.m_txHashSetManager.Close();
		return false;
	}

	lockedState.m_chainStore.Flush();

	return true;
}

bool TxHashSetProcessor::UpdateConfirmedChain(LockedChainState& lockedState, const BlockHeader& blockHeader)
{
	Chain& candidateChain = lockedState.m_chainStore.GetCandidateChain();
	Chain& confirmedChain = lockedState.m_chainStore.GetConfirmedChain();
	
	BlockIndex* pBlockIndex = candidateChain.GetByHeight(blockHeader.GetHeight());
	if (pBlockIndex->GetHash() != blockHeader.GetHash())
	{
		return false;
	}

	BlockIndex* pCommonIndex = lockedState.m_chainStore.FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED);
	if (confirmedChain.Rewind(pCommonIndex->GetHeight()))
	{
		uint64_t height = pCommonIndex->GetHeight() + 1;
		while (height <= pBlockIndex->GetHeight())
		{
			confirmedChain.AddBlock(candidateChain.GetByHeight(height));
			height++;
		}

		return true;
	}

	return false;
}