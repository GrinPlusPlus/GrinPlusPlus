#pragma once

#include "..//NodeContext.h"

#include <Wallet/NodeClient.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

class DefaultNodeClient : public INodeClient
{
public:
	DefaultNodeClient(
		IDatabasePtr pDatabase,
		TxHashSetManagerPtr pTxHashSetManager,
		ITransactionPoolPtr pTransactionPool,
		IBlockChainServerPtr pBlockChainServer,
		IP2PServerPtr pP2PServer)
		: m_pDatabase(pDatabase),
		m_pTxHashSetManager(pTxHashSetManager),
		m_pTransactionPool(pTransactionPool),
		m_pBlockChainServer(pBlockChainServer),
		m_pP2PServer(pP2PServer)
	{

	}

	static std::shared_ptr<DefaultNodeClient> Create(const Config& config)
	{
		IDatabasePtr pDatabase = DatabaseAPI::OpenDatabase(config);
		TxHashSetManagerPtr pTxHashSetManager = std::shared_ptr<TxHashSetManager>(new TxHashSetManager(config));
		ITransactionPoolPtr pTransactionPool = TxPoolAPI::CreateTransactionPool(config, pTxHashSetManager);
		IBlockChainServerPtr pBlockChainServer = BlockChainAPI::StartBlockChainServer(config, pDatabase->GetBlockDB(), pTxHashSetManager, pTransactionPool);
		IP2PServerPtr pP2PServer = P2PAPI::StartP2PServer(config, pBlockChainServer, pTxHashSetManager, pDatabase, pTransactionPool);

		return std::make_shared<DefaultNodeClient>(DefaultNodeClient(pDatabase, pTxHashSetManager, pTransactionPool, pBlockChainServer, pP2PServer));
	}
    
    virtual ~DefaultNodeClient() = default;

	std::shared_ptr<NodeContext> GetNodeContext()
	{
		return std::make_shared<NodeContext>(NodeContext(m_pDatabase, m_pBlockChainServer, m_pP2PServer, m_pTxHashSetManager, m_pTransactionPool));
	}

	virtual uint64_t GetChainHeight() const override final
	{
		return m_pBlockChainServer->GetHeight(EChainType::CONFIRMED);
	}

	virtual BlockHeaderPtr GetBlockHeader(const uint64_t height) const override final
	{
		return m_pBlockChainServer->GetBlockHeaderByHeight(height, EChainType::CONFIRMED);
	}

	virtual std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const override final
	{ 
		auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
		if (pTxHashSet == nullptr)
		{
			return std::map<Commitment, OutputLocation>();
		}

		std::map<Commitment, OutputLocation> outputs;
		for (const Commitment& commitment : commitments)
		{
			std::unique_ptr<OutputLocation> pOutputPosition = m_pDatabase->GetBlockDB()->Read()->GetOutputPosition(commitment);
			if (pOutputPosition != nullptr && pTxHashSet->Read()->IsUnspent(*pOutputPosition))
			{
				outputs.insert(std::make_pair(commitment, *pOutputPosition));
			}
		}

		return outputs;
	}

	virtual std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t startHeight, const uint64_t maxHeight) const override final
	{
		return m_pBlockChainServer->GetOutputsByHeight(startHeight, maxHeight);
	}

	virtual std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const override final
	{
		auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
		if (pTxHashSet == nullptr)
		{
			return std::unique_ptr<OutputRange>(nullptr);
		}

		auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
		return std::make_unique<OutputRange>(pTxHashSet->Read()->GetOutputsByLeafIndex(pBlockDB.GetShared(), startIndex, maxNumOutputs));
	}

	virtual bool PostTransaction(TransactionPtr pTransaction, const EPoolType poolType) override final
	{
		auto pTipHeader = m_pBlockChainServer->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pTipHeader != nullptr)
		{
			auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
			auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
			if (pTxHashSet != nullptr)
			{
				auto pTxHashSetReader = pTxHashSet->Read();
				try
				{
					auto result = m_pTransactionPool->AddTransaction(
						pBlockDB.GetShared(),
						pTxHashSetReader.GetShared(),
						pTransaction,
						poolType,
						*pTipHeader
					);
					
					if (result == EAddTransactionStatus::ADDED)
					{
						if (poolType == EPoolType::MEMPOOL)
						{
							m_pP2PServer->BroadcastTransaction(pTransaction);
						}

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

	IP2PServerPtr GetP2PServer() { return m_pP2PServer; }

private:
	IDatabasePtr m_pDatabase;
	TxHashSetManagerPtr m_pTxHashSetManager;
	ITransactionPoolPtr m_pTransactionPool;
	IBlockChainServerPtr m_pBlockChainServer;
	IP2PServerPtr m_pP2PServer;
};
