#pragma once

#include <Core/Context.h>
#include <Wallet/NodeClient.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

class TestNodeClient : public INodeClient
{
public:
	TestNodeClient(
		const IDatabasePtr& pDatabase,
		Locked<TxHashSetManager> pTxHashSetManager,
		const ITransactionPoolPtr& pTransactionPool,
		const IBlockChainServerPtr& pBlockChainServer)
		: m_pDatabase(pDatabase),
		m_pTxHashSetManager(pTxHashSetManager),
		m_pTransactionPool(pTransactionPool),
		m_pBlockChainServer(pBlockChainServer)
	{

	}
	virtual ~TestNodeClient() = default;

	uint64_t GetChainHeight() const final
	{
		return m_pBlockChainServer->GetHeight(EChainType::CONFIRMED);
	}

	BlockHeaderPtr GetBlockHeader(const uint64_t height) const final
	{
		return m_pBlockChainServer->GetBlockHeaderByHeight(height, EChainType::CONFIRMED);
	}

	std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const final
	{
		std::map<Commitment, OutputLocation> outputs;
		for (const Commitment& commitment : commitments)
		{
			std::unique_ptr<OutputLocation> pOutputPosition = m_pDatabase->GetBlockDB()->Read()->GetOutputPosition(commitment);
			if (pOutputPosition != nullptr)
			{
				outputs.insert(std::make_pair(commitment, *pOutputPosition));
			}
		}

		return outputs;
	}

	std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t startHeight, const uint64_t maxHeight) const final
	{
		return m_pBlockChainServer->GetOutputsByHeight(startHeight, maxHeight);
	}

	std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const final
	{
		auto pTxHashSet = m_pTxHashSetManager.Read()->GetTxHashSet();
		if (pTxHashSet == nullptr)
		{
			return std::unique_ptr<OutputRange>(nullptr);
		}

		auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
		return std::make_unique<OutputRange>(pTxHashSet->GetOutputsByLeafIndex(pBlockDB.GetShared(), startIndex, maxNumOutputs));
	}

	bool PostTransaction(TransactionPtr pTransaction, const EPoolType poolType) final
	{
		auto pTipHeader = m_pBlockChainServer->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pTipHeader != nullptr)
		{
			auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
			auto pTxHashSet = m_pTxHashSetManager.Read()->GetTxHashSet();
			if (pTxHashSet != nullptr)
			{
				try
				{
					auto result = m_pTransactionPool->AddTransaction(
						pBlockDB.GetShared(),
						pTxHashSet,
						pTransaction,
						poolType,
						*pTipHeader
					);

					if (result == EAddTransactionStatus::ADDED)
					{
						return true;
					}
				}
				catch (std::exception&)
				{
					return false;
				}
			}
		}

		return false;
	}

private:
	IDatabasePtr m_pDatabase;
	Locked<TxHashSetManager> m_pTxHashSetManager;
	ITransactionPoolPtr m_pTransactionPool;
	IBlockChainServerPtr m_pBlockChainServer;
};
