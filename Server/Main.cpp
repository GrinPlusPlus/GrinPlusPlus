#include "Node/NodeDaemon.h"
#include "Wallet/WalletDaemon.h"
#include "civetweb/include/civetweb.h"

#include <Wallet/WalletManager.h>
#include <Config/ConfigManager.h>
#include <Infrastructure/ThreadManager.h>

#include <windows.h> 
#include <stdio.h> 
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

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

int main(int argc, char* argv[])
{
	ThreadManagerAPI::SetCurrentThreadName("MAIN THREAD");

	std::cout << "INITIALIZING...\n";
	std::cout << std::flush;

	/* Initialize the civetweb library */
	mg_init_library(0);

	SetConsoleCtrlHandler(CtrlHandler, TRUE);

	const Config config = ConfigManager::LoadConfig();
	NodeDaemon node(config);
	INodeClient* pNodeClient = node.Initialize();

	WalletDaemon wallet(config, *pNodeClient);
	wallet.Initialize();

	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
	while (true)
	{
		if (SHUTDOWN)
		{
			break;
		}

		const int secondsRunning = (int)(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch() - startTime.time_since_epoch()).count());
		node.UpdateDisplay(secondsRunning);

		std::this_thread::sleep_for(std::chrono::seconds(1));
	}

	system("CLS");
	std::cout << "SHUTTING DOWN...";

	wallet.Shutdown();
	delete pNodeClient;
	node.Shutdown();

	/* Un-initialize the civetweb library */
	mg_exit_library();

	return 0;
}