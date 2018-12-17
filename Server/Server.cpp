#include "Server.h"

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
		// Handle the CTRL-C signal. 
	case CTRL_C_EVENT:
		printf("Ctrl-C event\n\n");
		SHUTDOWN = true;
		return TRUE;

	//	// CTRL-CLOSE: confirm that the user wants to exit. 
	//case CTRL_CLOSE_EVENT:
	//	Beep(600, 200);
	//	printf("Ctrl-Close event\n\n");
	//	return TRUE;

	//	// Pass other signals to the next handler. 
	//case CTRL_BREAK_EVENT:
	//	Beep(900, 200);
	//	printf("Ctrl-Break event\n\n");
	//	return TRUE;

	//case CTRL_LOGOFF_EVENT:
	//	Beep(1000, 200);
	//	printf("Ctrl-Logoff event\n\n");
	//	return FALSE;

	//case CTRL_SHUTDOWN_EVENT:
	//	Beep(750, 500);
	//	printf("Ctrl-Shutdown event\n\n");
	//	return FALSE;

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

	//int numSeconds;
	//std::cin >> numSeconds;
	//for (int i = 0; i < numSeconds; i++)
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
		std::cout << std::flush;

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	std::cout << "\nSHUTTING DOWN...";

	P2PAPI::ShutdownP2PServer(m_pP2PServer);
	BlockChainAPI::ShutdownBlockChainServer(m_pBlockChainServer);
	DatabaseAPI::CloseDatabase(m_pDatabase);
	LoggerAPI::Flush();
}