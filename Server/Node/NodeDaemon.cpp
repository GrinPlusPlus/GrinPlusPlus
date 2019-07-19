#include "NodeDaemon.h"
#include "NodeRestServer.h"
#include "NodeClients/DefaultNodeClient.h"

#include <P2P/P2PServer.h>
#include <Config/Config.h>
#include <Config/ConfigManager.h>
#include <BlockChain/BlockChainServer.h>
#include <Crypto/Crypto.h>
#include <Consensus/BlockDifficulty.h>
#include <Database/Database.h>
#include <Infrastructure/Logger.h>
#include <PMMR/TxHashSetManager.h>

#include <iostream>
#include <thread>

NodeDaemon::NodeDaemon(const Config& config)
	: m_config(config)
{

}

INodeClient* NodeDaemon::Initialize()
{
	LoggerAPI::Initialize(m_config.GetLogDirectory(), m_config.GetLogLevel());
	m_pDatabase = DatabaseAPI::OpenDatabase(m_config);
	m_pTxHashSetManager = new TxHashSetManager(m_config, m_pDatabase->GetBlockDB());
	m_pTransactionPool = TxPoolAPI::CreateTransactionPool(m_config, *m_pTxHashSetManager, m_pDatabase->GetBlockDB());
	m_pBlockChainServer = BlockChainAPI::StartBlockChainServer(m_config, *m_pDatabase, *m_pTxHashSetManager, *m_pTransactionPool);
	m_pP2PServer = P2PAPI::StartP2PServer(m_config, *m_pBlockChainServer, *m_pDatabase, *m_pTransactionPool);

	m_pNodeRestServer = new NodeRestServer(m_config, m_pDatabase, m_pTxHashSetManager, m_pBlockChainServer, m_pP2PServer);
	m_pNodeRestServer->Initialize();

	m_pNodeClient = new DefaultNodeClient(*m_pBlockChainServer, m_pDatabase->GetBlockDB(), *m_pTxHashSetManager, *m_pTransactionPool);
	return m_pNodeClient;
}

void NodeDaemon::Shutdown()
{
	try
	{
		delete m_pNodeClient;
		m_pNodeRestServer->Shutdown();
		P2PAPI::ShutdownP2PServer(m_pP2PServer);
		TxPoolAPI::DestroyTransactionPool(m_pTransactionPool);
		BlockChainAPI::ShutdownBlockChainServer(m_pBlockChainServer);
		delete m_pTxHashSetManager;
		DatabaseAPI::CloseDatabase(m_pDatabase);
		LoggerAPI::Flush();
	}
	catch(const std::system_error& e)
	{
		std::cerr << e.what() << '\n';
		std::cerr << "FAILURE" << '\n';
	}

}

void NodeDaemon::UpdateDisplay(const int secondsRunning)
{
	const SyncStatus& syncStatus = m_pP2PServer->GetSyncStatus();

#ifdef _WIN32
		std::system("cls");
#else
		std::system("clear");
#endif

	std::cout << "Time Running: " << secondsRunning << "s";

	const ESyncStatus status = syncStatus.GetStatus();
	if (status == ESyncStatus::NOT_SYNCING)
	{
		std::cout << "\nStatus: Running";
	}
	else if (status == ESyncStatus::WAITING_FOR_PEERS)
	{
		std::cout << "\nStatus: Waiting for Peers";
	}
	else if (status == ESyncStatus::SYNCING_HEADERS)
	{
		const uint64_t networkHeight = syncStatus.GetNetworkHeight();
		const uint64_t percentage = networkHeight > 0 ? (syncStatus.GetHeaderHeight() * 100 / networkHeight) : 0;
		std::cout << "\nStatus: Syncing Headers (" << percentage << "%)";
	}
	else if (status == ESyncStatus::SYNCING_TXHASHSET)
	{
		const uint64_t downloaded = syncStatus.GetDownloaded();
		const uint64_t downloadSize = syncStatus.GetDownloadSize();
		const uint64_t percentage = downloadSize > 0 ? (downloaded * 100) / downloadSize : 0;

		std::cout << "\nStatus: Syncing TxHashSet " << downloaded << "/" << downloadSize << "(" << percentage << "%)";
	}
	else if (status == ESyncStatus::PROCESSING_TXHASHSET)
	{
		std::cout << "\nStatus: Validating TxHashSet";
	}
	else if (status == ESyncStatus::TXHASHSET_SYNC_FAILED)
	{
		std::cout << "\nStatus: TxHashSet Sync Failed - Trying Again";
	}
	else if (status == ESyncStatus::SYNCING_BLOCKS)
	{
		std::cout << "\nStatus: Syncing blocks";
	}

	std::cout << "\nNumConnections: " << syncStatus.GetNumActiveConnections();
	std::cout << "\nHeader Height: " << syncStatus.GetHeaderHeight();
	std::cout << "\nHeader Difficulty: " << syncStatus.GetHeaderDifficulty();
	std::cout << "\nBlock Height: " << syncStatus.GetBlockHeight();
	std::cout << "\nBlock Difficulty: " << syncStatus.GetBlockDifficulty();
	std::cout << "\nNetwork Height: " << syncStatus.GetNetworkHeight();
	std::cout << "\nNetwork Difficulty: " << syncStatus.GetNetworkDifficulty();
	std::cout << "\n\nPress Ctrl-C to exit...";
	std::cout << std::flush;
}