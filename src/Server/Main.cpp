#include "Node/NodeDaemon.h"
#include "Wallet/WalletDaemon.h"
#include "civetweb/include/civetweb.h"

#include <Wallet/WalletManager.h>
#include <Config/ConfigLoader.h>
#include <Infrastructure/ShutdownManager.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/ThreadUtil.h>

#include <signal.h>
#include <stdio.h> 
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

void StartServer(const Config& config, const bool headless);

static void SigIntHandler(int signum)
{
	printf("\n\nCtrl-C Pressed\n\n");
	ShutdownManagerAPI::Shutdown();
}

int main(int argc, char* argv[])
{
	ThreadManagerAPI::SetCurrentThreadName("MAIN");

	EEnvironmentType environment = EEnvironmentType::MAINNET;
	bool headless = false;
	for (int i = 0; i < argc; ++i)
	{
		if (std::string(argv[i]) == "--floonet")
		{
			environment = EEnvironmentType::FLOONET;
		}
		if (std::string(argv[i]) == "--headless")
		{
			headless = true;
		}
	}

	if (!headless)
	{
		std::cout << "INITIALIZING...\n";
		std::cout << std::flush;
	}

	signal(SIGINT, SigIntHandler);
	signal(SIGTERM, SigIntHandler);
	signal(SIGABRT, SigIntHandler);
	signal(9, SigIntHandler);

	const ConfigPtr pConfig = ConfigLoader::Load(environment);
	LoggerAPI::Initialize(pConfig->GetLogDirectory().u8string(), pConfig->GetLogLevel());
	mg_init_library(0);

	StartServer(*pConfig, headless);

	mg_exit_library();
	LoggerAPI::Flush();

	return 0;
}

void StartServer(const Config& config, const bool headless)
{
	std::shared_ptr<NodeDaemon> pNode = NodeDaemon::Create(config);
	std::shared_ptr<WalletDaemon> pWallet = WalletDaemon::Create(config, pNode->GetNodeClient());

	std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
	while (true)
	{
		if (ShutdownManagerAPI::WasShutdownRequested())
		{
			break;
		}

		if (!headless)
		{
			const int secondsRunning = (int)(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch() - startTime.time_since_epoch()).count());
			pNode->UpdateDisplay(secondsRunning);
		}

		ThreadUtil::SleepFor(std::chrono::seconds(1), ShutdownManagerAPI::WasShutdownRequested());
	}

	if (!headless)
	{
#ifdef _WIN32
		std::system("cls");
#else
		std::system("clear");
#endif

		std::cout << "SHUTTING DOWN...";
	}
}