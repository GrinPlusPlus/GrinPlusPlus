#include "TxHashSetProcessor.h"

#include <Infrastructure/Logger.h>
#include <Database/BlockDb.h>
#include <Core/BlockHeader.h>
#include <BlockChainServer.h>
#include <StringUtil.h>
#include <HexUtil.h>

TxHashSetProcessor::TxHashSetProcessor(const Config& config, IBlockChainServer& blockChainServer, ChainState& chainState, IBlockDB& blockDB)
	: m_config(config), m_blockChainServer(blockChainServer), m_chainState(chainState), m_blockDB(blockDB)
{

}

ITxHashSet* TxHashSetProcessor::ProcessTxHashSet(const Hash& blockHash, const std::string& path)
{
	std::unique_ptr<BlockHeader> pHeader = m_chainState.GetBlockHeaderByHash(blockHash);
	if (pHeader == nullptr)
	{
		LoggerAPI::LogError(StringUtil::Format("TxHashSetProcessor::ProcessTxHashSet - Header not found for hash %s.", HexUtil::ConvertHash(blockHash)));
		return nullptr;
	}

	// 1. Load and Extract TxHashSet Zip
	ITxHashSet* pTxHashSet = TxHashSetAPI::LoadFromZip(m_config, m_blockDB, path, *pHeader);
	if (pTxHashSet == nullptr)
	{
		LoggerAPI::LogError("TxHashSetProcessor::ProcessTxHashSet - Failed to load " + path);
		return nullptr;
	}

	// 2. Validate entire TxHashSet
	Commitment outputSum(CBigInteger<33>::ValueOf(0));
	Commitment kernelSum(CBigInteger<33>::ValueOf(0));
	if (!pTxHashSet->Validate(*pHeader, m_blockChainServer, outputSum, kernelSum))
	{
		LoggerAPI::LogError(StringUtil::Format("TxHashSetProcessor::ProcessTxHashSet - Validation of %s failed.", path.c_str()));
		TxHashSetAPI::Close(pTxHashSet);
		return nullptr;
	}

	// 3. Add BlockSums to DB
	const BlockSums blockSums(std::move(outputSum), std::move(kernelSum));
	m_blockDB.AddBlockSums(pHeader->GetHash(), blockSums);

	// 4. Add Output positions to DB
	pTxHashSet->SaveOutputPositions();

	// 5. Update confirmed chain
	if (!UpdateConfirmedChain(*pHeader))
	{
		LoggerAPI::LogError(StringUtil::Format("TxHashSetProcessor::ProcessTxHashSet - Failed to update confirmed chain for %s.", path.c_str()));
		TxHashSetAPI::Close(pTxHashSet);
		return nullptr;
	}

	// TODO: 6. Check for orphans

	return pTxHashSet;
}

bool TxHashSetProcessor::UpdateConfirmedChain(const BlockHeader& blockHeader)
{
	LockedChainState lockedState = m_chainState.GetLocked();
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