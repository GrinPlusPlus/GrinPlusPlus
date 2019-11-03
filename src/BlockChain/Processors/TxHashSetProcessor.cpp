#include "TxHashSetProcessor.h"

#include <Infrastructure/Logger.h>
#include <Database/BlockDb.h>
#include <Core/Models/BlockHeader.h>
#include <BlockChain/BlockChainServer.h>
#include <Common/Util/StringUtil.h>
#include <Common/Util/HexUtil.h>

TxHashSetProcessor::TxHashSetProcessor(
	const Config& config,
	IBlockChainServer& blockChainServer,
	std::shared_ptr<Locked<ChainState>> pChainState,
	std::shared_ptr<Locked<IBlockDB>> pBlockDB)
	: m_config(config),
	m_blockChainServer(blockChainServer),
	m_pChainState(pChainState),
	m_pBlockDB(pBlockDB)
{

}

bool TxHashSetProcessor::ProcessTxHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus)
{
	std::unique_ptr<BlockHeader> pHeader = m_pChainState->Read()->GetBlockHeaderByHash(blockHash);
	if (pHeader == nullptr)
	{
		LOG_ERROR_F("Header not found for hash %s.", blockHash);
		return false;
	}

	// 1. Close Existing TxHashSet
	{
		auto pChainState = m_pChainState->BatchWrite();
		pChainState->GetTxHashSetManager()->Close();
		pChainState->Commit();
	}

	// 2. Load and Extract TxHashSet Zip
	ITxHashSetPtr pTxHashSet = TxHashSetManager::LoadFromZip(m_config, m_pBlockDB, path, *pHeader);
	if (pTxHashSet == nullptr)
	{
		LOG_ERROR_F("Failed to load %s", path);
		return false;
	}

	// 3. Validate entire TxHashSet
	std::unique_ptr<BlockSums> pBlockSums = pTxHashSet->ValidateTxHashSet(*pHeader, m_blockChainServer, syncStatus);
	if (pBlockSums == nullptr)
	{
		LOG_ERROR_F("Validation of %s failed.", path);
		return false;
	}

	// 4. Add BlockSums to DB
	auto pChainStateBatch = m_pChainState->BatchWrite();

	pChainStateBatch->GetBlockDB()->AddBlockSums(pHeader->GetHash(), *pBlockSums);

	// 5. Add Output positions to DB
	{
		LOG_DEBUG("Saving output positions.");
		std::shared_ptr<Chain> pCandidateChain = pChainStateBatch->GetChainStore()->GetCandidateChain();

		uint64_t firstOutput = 0;
		for (uint64_t i = 0; i <= pHeader->GetHeight(); i++)
		{
			std::unique_ptr<BlockHeader> pNextHeader = pChainStateBatch->GetBlockHeaderByHeight(i, EChainType::CANDIDATE);
			if (pNextHeader != nullptr)
			{
				pTxHashSet->SaveOutputPositions(pChainStateBatch->GetBlockDB(), *pNextHeader, firstOutput);
				firstOutput = pNextHeader->GetOutputMMRSize();
			}
		}
	}

	// 6. Store TxHashSet
	LOG_DEBUG("Using TxHashSet.");
	pChainStateBatch->GetTxHashSetManager()->SetTxHashSet(pTxHashSet);

	// 7. Update confirmed chain
	LOG_DEBUG("Updating confirmed chain.");
	if (!UpdateConfirmedChain(pChainStateBatch, *pHeader))
	{
		LOG_ERROR_F("Failed to update confirmed chain for %s.", path);
		pChainStateBatch->GetTxHashSetManager()->Close();
		return false;
	}

	pChainStateBatch->Commit();

	return true;
}

bool TxHashSetProcessor::UpdateConfirmedChain(Writer<ChainState> pLockedState, const BlockHeader& blockHeader)
{
	std::shared_ptr<Chain> pCandidateChain = pLockedState->GetChainStore()->GetCandidateChain();
	std::shared_ptr<Chain> pConfirmedChain = pLockedState->GetChainStore()->GetConfirmedChain();
	
	std::shared_ptr<const BlockIndex> pBlockIndex = pCandidateChain->GetByHeight(blockHeader.GetHeight());
	if (pBlockIndex->GetHash() != blockHeader.GetHash())
	{
		return false;
	}

	std::shared_ptr<const BlockIndex> pCommonIndex = pLockedState->GetChainStore()->FindCommonIndex(EChainType::CANDIDATE, EChainType::CONFIRMED);
	pConfirmedChain->Rewind(pCommonIndex->GetHeight());

	uint64_t height = pCommonIndex->GetHeight() + 1;
	while (height <= pBlockIndex->GetHeight())
	{
		pConfirmedChain->AddBlock(pCandidateChain->GetHash(height));
		height++;
	}

	return true;
}