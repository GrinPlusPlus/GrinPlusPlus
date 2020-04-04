#pragma once

#include "TestHelper.h"

#include <Core/File/FileRemover.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

class TestServer
{
public:
	using Ptr = std::shared_ptr<TestServer>;

	static TestServer::Ptr Create()
	{
		ConfigPtr pConfig = TestHelper::GetTestConfig();
		IDatabasePtr pDatabase = DatabaseAPI::OpenDatabase(*pConfig);
		auto pTxHashSetManager = std::make_shared<Locked<TxHashSetManager>>(std::make_shared<TxHashSetManager>(*pConfig));
		ITransactionPoolPtr pTxPool = TxPoolAPI::CreateTransactionPool(*pConfig);
		auto pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(*pConfig);
		IBlockChainServerPtr pBlockChainServer = BlockChainAPI::StartBlockChainServer(
			*pConfig,
			pDatabase->GetBlockDB(),
			pTxHashSetManager,
			pTxPool,
			pHeaderMMR
		);

		return std::make_shared<TestServer>(
			pConfig,
			pDatabase,
			pTxHashSetManager,
			pTxPool,
			pHeaderMMR,
			pBlockChainServer
		);
	}

	TestServer(
		const ConfigPtr& pConfig,
		const IDatabasePtr& pDatabase,
		const std::shared_ptr<Locked<TxHashSetManager>>& pTxHashSetManager,
		const ITransactionPoolPtr& pTxPool,
		const std::shared_ptr<Locked<IHeaderMMR>>& pHeaderMMR,
		const IBlockChainServerPtr& pBlockChainServer
	) : m_pConfig(pConfig),
		m_pDatabase(pDatabase),
		m_pTxHashSetManager(pTxHashSetManager),
		m_pTxPool(pTxPool),
		m_pHeaderMMR(pHeaderMMR),
		m_pBlockChainServer(pBlockChainServer)
	{

	}

	~TestServer()
	{
		FileUtil::RemoveFile(m_pConfig->GetDataDirectory());
	}

	const ConfigPtr& GetConfig() const noexcept { return m_pConfig; }
	const FullBlock& GetGenesisBlock() const noexcept { return m_pConfig->GetEnvironment().GetGenesisBlock(); }
	const BlockHeaderPtr& GetGenesisHeader() const noexcept { return m_pConfig->GetEnvironment().GetGenesisHeader(); }

	const IDatabasePtr& GetDatabase() const noexcept { return m_pDatabase; }
	const std::shared_ptr<Locked<TxHashSetManager>>& GetTxHashSetManager() const noexcept { return m_pTxHashSetManager; }
	const ITransactionPoolPtr& GetTxPool() const noexcept { return m_pTxPool; }
	const std::shared_ptr<Locked<IHeaderMMR>>& GetHeaderMMR() const noexcept { return m_pHeaderMMR; }
	const IBlockChainServerPtr& GetBlockChainServer() const noexcept { return m_pBlockChainServer; }

private:
	ConfigPtr m_pConfig;
	IDatabasePtr m_pDatabase;
	std::shared_ptr<Locked<TxHashSetManager>> m_pTxHashSetManager;
	ITransactionPoolPtr m_pTxPool;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
	IBlockChainServerPtr m_pBlockChainServer;
};