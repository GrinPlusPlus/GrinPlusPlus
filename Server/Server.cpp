#include "Server.h"
#include "NodeRestServer.h"

#include <P2PServer.h>
#include <Config/Config.h>
#include <Config/ConfigManager.h>
#include <BlockChainServer.h>
#include <Crypto.h>
#include <Consensus/BlockDifficulty.h>
#include <Database/Database.h>
#include <Infrastructure/Logger.h>
#include <Infrastructure/ThreadManager.h>
#include <PMMR/TxHashSetManager.h>

#include <iostream>
#include <thread>
#include <chrono>

#include <windows.h> 
#include <stdio.h> 

std::atomic_bool SHUTDOWN = false;

BOOL WINAPI CtrlHandler(DWORD fdwCtrlType)
{
	switch (fdwCtrlType)
	{
	case CTRL_C_EVENT:
		printf("\n\nCtrl-C Pressed\n\n");
		SHUTDOWN = true;
		return TRUE;

	default:
		return FALSE;
	}
}

Server::Server()
	: m_config(ConfigManager::LoadConfig())
{

}

void Server::Run()
{
	ThreadManagerAPI::SetCurrentThreadName("MAIN THREAD");

	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	m_config = ConfigManager::LoadConfig();
	m_pDatabase = DatabaseAPI::OpenDatabase(m_config);
	m_pTxHashSetManager = new TxHashSetManager(m_config, m_pDatabase->GetBlockDB());
	m_pBlockChainServer = BlockChainAPI::StartBlockChainServer(m_config, *m_pDatabase, *m_pTxHashSetManager);
	m_pP2PServer = P2PAPI::StartP2PServer(m_config, *m_pBlockChainServer, *m_pDatabase);

	NodeRestServer nodeRestServer(m_config, m_pDatabase, m_pTxHashSetManager, m_pBlockChainServer, m_pP2PServer);
	nodeRestServer.Start();

	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
	while(true)
	{
		if (SHUTDOWN)
		{
			break;
		}

		const SyncStatus& syncStatus = m_pP2PServer->GetSyncStatus();

		system("CLS");
		std::cout << "Time Running: " << std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch() - startTime.time_since_epoch()).count() << "s";

		const ESyncStatus status = syncStatus.GetStatus();
		if (status == ESyncStatus::NOT_SYNCING)
		{
			std::cout << "\nStatus: Running";
		}
		else if (status == ESyncStatus::SYNCING_HEADERS)
		{
			std::cout << "\nStatus: Syncing Headers";
		}
		else if (status == ESyncStatus::SYNCING_TXHASHSET)
		{
			const uint64_t downloaded = syncStatus.GetDownloaded();
			const uint64_t downloadSize = syncStatus.GetDownloadSize();
			const uint64_t percentage = (downloaded * 100) / downloadSize;

			std::cout << "\nStatus: Syncing TxHashSet " << downloaded << "/" << downloadSize << "(" << percentage << "%)";
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

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::cout << "\nSHUTTING DOWN...";

	nodeRestServer.Shutdown();

	P2PAPI::ShutdownP2PServer(m_pP2PServer);
	BlockChainAPI::ShutdownBlockChainServer(m_pBlockChainServer);
	delete m_pTxHashSetManager;
	DatabaseAPI::CloseDatabase(m_pDatabase);
	LoggerAPI::Flush();
}