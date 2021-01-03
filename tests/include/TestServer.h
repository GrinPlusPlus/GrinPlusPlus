#pragma once

#include <TestHelper.h>
#include <TestNodeClient.h>

#include <Core/File/FileRemover.h>
#include <BlockChain/BlockChain.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>
#include <Crypto/CSPRNG.h>
#include <thread>

class TestServer
{
public:
	using Ptr = std::shared_ptr<TestServer>;

	static TestServer::Ptr Create()
	{
		ConfigPtr pConfig = TestHelper::GetTestConfig();
		LoggerAPI::Initialize(pConfig->GetLogDirectory(), pConfig->GetLogLevel());
		IDatabasePtr pDatabase = DatabaseAPI::OpenDatabase(*pConfig);
		auto pTxHashSetManager = std::make_shared<Locked<TxHashSetManager>>(std::make_shared<TxHashSetManager>(*pConfig));
		ITransactionPool::Ptr pTxPool = TxPoolAPI::CreateTransactionPool(*pConfig);
		auto pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(*pConfig);
		IBlockChain::Ptr pBlockChain = BlockChainAPI::OpenBlockChain(
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
			pBlockChain
		);
	}

	TestServer(
		const ConfigPtr& pConfig,
		const IDatabasePtr& pDatabase,
		const std::shared_ptr<Locked<TxHashSetManager>>& pTxHashSetManager,
		const ITransactionPool::Ptr& pTxPool,
		const std::shared_ptr<Locked<IHeaderMMR>>& pHeaderMMR,
		const IBlockChain::Ptr& pBlockChain
	) : m_pConfig(pConfig),
		m_pDatabase(pDatabase),
		m_pTxHashSetManager(pTxHashSetManager),
		m_pTxPool(pTxPool),
		m_pHeaderMMR(pHeaderMMR),
		m_pBlockChain(pBlockChain),
		m_tempDirs(0)
	{

	}

	~TestServer()
	{
		auto dataPath = m_pConfig->GetDataDirectory();
		m_pConfig.reset();
		m_pDatabase.reset();
		m_pTxHashSetManager.reset();
		m_pTxPool.reset();
		m_pHeaderMMR.reset();
		m_pBlockChain.reset();
		FileUtil::RemoveFile(dataPath / "NODE" / "CHAIN");
		FileUtil::RemoveFile(dataPath / "NODE" / "DB");
		FileUtil::RemoveFile(dataPath / "NODE" / "TXHASHSET");
		FileUtil::RemoveFile(dataPath / "NODE");
		FileUtil::RemoveFile(dataPath / "WALLET");
	}

	const ConfigPtr& GetConfig() const noexcept { return m_pConfig; }
	const FullBlock& GetGenesisBlock() const noexcept { return Global::GetGenesisBlock(); }
	const BlockHeaderPtr& GetGenesisHeader() const noexcept { return Global::GetGenesisHeader(); }

	const IDatabasePtr& GetDatabase() const noexcept { return m_pDatabase; }
	const std::shared_ptr<Locked<TxHashSetManager>>& GetTxHashSetManager() const noexcept { return m_pTxHashSetManager; }
	const ITransactionPool::Ptr& GetTxPool() const noexcept { return m_pTxPool; }
	const std::shared_ptr<Locked<IHeaderMMR>>& GetHeaderMMR() const noexcept { return m_pHeaderMMR; }
	const IBlockChain::Ptr& GetBlockChain() const noexcept { return m_pBlockChain; }
	std::shared_ptr<INodeClient> GetNodeClient() const noexcept {
		return std::shared_ptr<INodeClient>(new TestNodeClient(m_pDatabase, *m_pTxHashSetManager, m_pTxPool, m_pBlockChain));
	}

	fs::path GenerateTempDir()
	{ 
		fs::path path = m_pConfig->GetDataDirectory() / "TEMP" / std::to_string(m_tempDirs++);
		fs::create_directories(path);
		return path;
	}

private:
	ConfigPtr m_pConfig;
	IDatabasePtr m_pDatabase;
	std::shared_ptr<Locked<TxHashSetManager>> m_pTxHashSetManager;
	ITransactionPool::Ptr m_pTxPool;
	std::shared_ptr<Locked<IHeaderMMR>> m_pHeaderMMR;
	IBlockChain::Ptr m_pBlockChain;

	size_t m_tempDirs;
};