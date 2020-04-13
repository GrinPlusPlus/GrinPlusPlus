#include "Node/NodeDaemon.h"
#include "Wallet/WalletDaemon.h"
#include <civetweb.h>

#include <Core/Context.h>
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

using namespace std::chrono;

void StartServer(const ConfigPtr& pConfig, const bool headless);

static void SigIntHandler(int signum)
{
	printf("\n\n%d signal received\n\n", signum);
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

	ConfigPtr pConfig = nullptr;
	try
	{
		pConfig = ConfigLoader::Load(environment);
	}
	catch (std::exception& e)
	{
		std::cerr << "Failed to open config: \n" << e.what() << std::endl;
		throw;
	}

	try
	{
		LoggerAPI::Initialize(pConfig->GetLogDirectory(), pConfig->GetLogLevel());
	}
	catch (std::exception & e)
	{
		std::cerr << "Failed to initialize logger: \n" << e.what() << std::endl;
		throw;
	}

	try
	{
		mg_init_library(0);

		StartServer(pConfig, headless);

		mg_exit_library();
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
		LoggerAPI::Flush();

		std::cerr << e.what() << std::endl;
		throw;
	}

	LoggerAPI::Flush();

	return 0;
}

void StartServer(const ConfigPtr& pConfig, const bool headless)
{
	Context::Ptr pContext = nullptr;
	try
	{
		pContext = Context::Create(pConfig);
	}
	catch (std::exception & e)
	{
		std::cerr << "Failed to create context: \n" << e.what() << std::endl;
		throw;
	}

	std::shared_ptr<NodeDaemon> pNode = NodeDaemon::Create(pContext);
	std::shared_ptr<WalletDaemon> pWallet = WalletDaemon::Create(pContext->GetConfig(), pNode->GetNodeClient());

	system_clock::time_point startTime = system_clock::now();
	while (true)
	{
		if (ShutdownManagerAPI::WasShutdownRequested())
		{
			if (!headless)
			{
#ifdef _WIN32
				std::system("cls");
#else
				std::system("clear");
#endif

				std::cout << "SHUTTING DOWN...";
			}

			break;
		}

		if (!headless)
		{
			const int secondsRunning = (int)(duration_cast<seconds>(system_clock::now().time_since_epoch() - startTime.time_since_epoch()).count());
			pNode->UpdateDisplay(secondsRunning);
		}

		ThreadUtil::SleepFor(seconds(1), ShutdownManagerAPI::WasShutdownRequested());
	}
}