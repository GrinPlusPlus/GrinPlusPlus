#include "Server.h"
#include "RestServer.h"

#include <P2PServer.h>
#include <Config/Config.h>
#include <Config/ConfigManager.h>
#include <BlockChainServer.h>
#include <Crypto.h>
#include <Consensus/BlockDifficulty.h>
#include <Database.h>
#include <Infrastructure/Logger.h>
#include <Infrastructure/ThreadManager.h>

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
	m_pBlockChainServer = BlockChainAPI::StartBlockChainServer(m_config, *m_pDatabase);
	m_pP2PServer = P2PAPI::StartP2PServer(m_config, *m_pBlockChainServer, *m_pDatabase);

	RestServer restServer(m_config, m_pDatabase, m_pBlockChainServer, m_pP2PServer);
	restServer.Start();

	int secondsRunning = 0;
	while(true)
	{
		if (SHUTDOWN)
		{
			break;
		}

		const size_t numConnections = m_pP2PServer->GetNumberOfConnectedPeers();
		const SyncStatus& syncStatus = m_pP2PServer->GetSyncStatus();

		system("CLS");
		std::cout << "Time Running: " << secondsRunning++ << "s";
		std::cout << "\nNumConnections: " << numConnections ;
		std::cout << "\nHeader Height: " << syncStatus.GetHeaderHeight();
		std::cout << "\nHeader Difficulty: " << syncStatus.GetHeaderDifficulty();
		std::cout << "\nBlock Height: " << syncStatus.GetBlockHeight();
		std::cout << "\nBlock Difficulty: " << syncStatus.GetBlockDifficulty();
		std::cout << "\n\nPress Ctrl-C to exit...";
		std::cout << std::flush;

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::cout << "\nSHUTTING DOWN...";

	restServer.Shutdown();

	P2PAPI::ShutdownP2PServer(m_pP2PServer);
	BlockChainAPI::ShutdownBlockChainServer(m_pBlockChainServer);
	DatabaseAPI::CloseDatabase(m_pDatabase);
	LoggerAPI::Flush();
}