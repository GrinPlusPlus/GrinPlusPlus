#pragma once

#include "../NodeContext.h"

#include <Core/Context.h>
#include <Wallet/NodeClient.h>
#include <BlockChain/BlockChain.h>
#include <Database/Database.h>
#include <Database/BlockDb.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

class DefaultNodeClient : public INodeClient
{
public:
	DefaultNodeClient(
		IDatabasePtr pDatabase,
		const TxHashSetManager::Ptr& pTxHashSetManager,
		const ITransactionPool::Ptr& pTransactionPool,
		const IBlockChain::Ptr& pBlockChain,
		IP2PServerPtr pP2PServer)
		: m_pDatabase(pDatabase),
		m_pTxHashSetManager(pTxHashSetManager),
		m_pTransactionPool(pTransactionPool),
		m_pBlockChain(pBlockChain),
		m_pP2PServer(pP2PServer)
	{

	}

	static std::shared_ptr<DefaultNodeClient> Create(const Context::Ptr& pContext)
	{
		auto pDatabase = DatabaseAPI::OpenDatabase(pContext->GetConfig());
		auto pTxHashSetManager = std::make_shared<TxHashSetManager>(pContext->GetConfig());
		auto pLockedTxHashSetManager = std::make_shared<Locked<TxHashSetManager>>(pTxHashSetManager);
		auto pTransactionPool = TxPoolAPI::CreateTransactionPool(pContext->GetConfig());
		auto pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(pContext->GetConfig());
		auto pBlockChainServer = BlockChainAPI::OpenBlockChain(
			pContext->GetConfig(),
			pDatabase->GetBlockDB(),
			pLockedTxHashSetManager,
			pTransactionPool,
			pHeaderMMR
		);
		auto pP2PServer = P2PAPI::StartP2PServer(
			pContext,
			pBlockChainServer,
			pLockedTxHashSetManager,
			pDatabase,
			pTransactionPool
		);

		return std::make_shared<DefaultNodeClient>(
			pDatabase,
			pTxHashSetManager,
			pTransactionPool,
			pBlockChainServer,
			pP2PServer
		);
	}
    
    virtual ~DefaultNodeClient() = default;

	std::shared_ptr<NodeContext> GetNodeContext()
	{
		return std::make_shared<NodeContext>(NodeContext{
			m_pDatabase,
			m_pBlockChain,
			m_pP2PServer,
			m_pTxHashSetManager,
			m_pTransactionPool
		});
	}

	uint64_t GetChainHeight() const final
	{
		return m_pBlockChain->GetHeight(EChainType::CONFIRMED);
	}

	BlockHeaderPtr GetTipHeader() const final
	{
		return m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
	}

	BlockHeaderPtr GetBlockHeader(const uint64_t height) const final
	{
		return m_pBlockChain->GetBlockHeaderByHeight(height, EChainType::CONFIRMED);
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
		return m_pBlockChain->GetOutputsByHeight(startHeight, maxHeight);
	}

	std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const final
	{
		auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
		if (pTxHashSet == nullptr) {
			return std::unique_ptr<OutputRange>(nullptr);
		}

		auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
		return std::make_unique<OutputRange>(
			pTxHashSet->GetOutputsByLeafIndex(pBlockDB.GetShared(), startIndex, maxNumOutputs)
		);
	}

	bool PostTransaction(TransactionPtr pTransaction, const EPoolType poolType) final
	{
		auto pTipHeader = m_pBlockChain->GetTipBlockHeader(EChainType::CONFIRMED);
		if (pTipHeader != nullptr) {
			auto pBlockDB = m_pDatabase->GetBlockDB()->Read();
			auto pTxHashSet = m_pTxHashSetManager->GetTxHashSet();
			if (pTxHashSet != nullptr) {
				try
				{
					auto result = m_pTransactionPool->AddTransaction(
						pBlockDB.GetShared(),
						pTxHashSet,
						pTransaction,
						poolType,
						*pTipHeader
					);
					
					if (result == EAddTransactionStatus::ADDED) {
						if (poolType == EPoolType::MEMPOOL) {
							m_pP2PServer->BroadcastTransaction(pTransaction);
						}

						return true;
					} else {
						LOG_ERROR_F("Failed to post {} for reason '{}'", pTransaction, (uint8_t)result);
					}
				}
				catch (std::exception& e)
				{
					LOG_ERROR_F("Failed to post {}. Exception: ", pTransaction, e.what());
					return false;
				}
			}
		}

		return false;
	}

	IP2PServerPtr GetP2PServer() { return m_pP2PServer; }

private:
	IDatabasePtr m_pDatabase;
	TxHashSetManager::Ptr m_pTxHashSetManager;
	ITransactionPool::Ptr m_pTransactionPool;
	IBlockChain::Ptr m_pBlockChain;
	IP2PServerPtr m_pP2PServer;
};
