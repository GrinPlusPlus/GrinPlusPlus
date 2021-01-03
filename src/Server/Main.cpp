#include "Node/Node.h"
#include "Node/NodeClients/RPCNodeClient.h"
#include "Wallet/WalletDaemon.h"
#include "opt.h"
#include "console.h"

#include <GrinVersion.h>
#include <Core/Context.h>
#include <Core/Global.h>
#include <Wallet/WalletManager.h>
#include <Config/ConfigLoader.h>
#include <Common/Logger.h>
#include <Common/Util/ThreadUtil.h>

#include <stdio.h> 
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>

using namespace std::chrono;

ConfigPtr Initialize(const EEnvironmentType environment, const bool headless);
void Run(const ConfigPtr& pConfig, const Options& options);

int main(int argc, char* argv[])
{
	LoggerAPI::SetThreadName("MAIN");

	Options opt = ParseOptions(argc, argv);
	if (opt.help)
	{
		PrintHelp();
		return 0;
	}

	ConfigPtr pConfig = Initialize(opt.environment, opt.headless);

	try
	{
		Run(pConfig, opt);
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Exception thrown: {}", e.what());
		LoggerAPI::Flush();
		IO::Err("Exception thrown", e);
		throw;
	}

	LoggerAPI::Flush();

	return 0;
}

ConfigPtr Initialize(const EEnvironmentType environment, const bool headless)
{
	if (!headless)
	{
		IO::Out("INITIALIZING...");
		IO::Flush();
	}

	ConfigPtr pConfig = nullptr;
	try
	{
		pConfig = ConfigLoader::Load(environment);
	}
	catch (std::exception& e)
	{
		IO::Err("Failed to open config", e);
		throw;
	}

	try
	{
		LoggerAPI::Initialize(pConfig->GetLogDirectory(), pConfig->GetLogLevel());
	}
	catch (std::exception& e)
	{
		IO::Err("Failed to initialize logger", e);
		throw;
	}

	return pConfig;
}

void Run(const ConfigPtr& pConfig, const Options& options)
{
	LOG_INFO_F("Starting Grin++ v{}", GRINPP_VERSION);

	Context::Ptr pContext = nullptr;

	try
	{
		pContext = Context::Create(pConfig);
		Global::Init(pContext);
	}
	catch (std::exception& e)
	{
		IO::Err("Failed to initialize global context", e);
		throw;
	}


	std::unique_ptr<Node> pNode = nullptr;
	INodeClientPtr pNodeClient = nullptr;

	if (options.shared_node.has_value())
	{
		pNodeClient = RPCNodeClient::Create(
			options.shared_node.value().first,
			options.shared_node.value().second
		);
	}
	else
	{
		pNode = Node::Create(pContext);
		pNodeClient = pNode->GetNodeClient();
	}

	std::unique_ptr<WalletDaemon> pWallet = nullptr;
	if (options.include_wallet)
	{
		pWallet = WalletDaemon::Create(
			pContext->GetConfig(),
			pContext->GetTorProcess(),
			pNodeClient
		);
	}

	system_clock::time_point startTime = system_clock::now();
	while (true)
	{
		if (!Global::IsRunning())
		{
			if (!options.headless)
			{
				IO::Clear();
				IO::Out("SHUTTING DOWN...");
			}

			break;
		}

		if (pNode != nullptr && !options.headless)
		{
			auto duration = system_clock::now().time_since_epoch() - startTime.time_since_epoch();
			const int secondsRunning = (int)(duration_cast<seconds>(duration).count());
			pNode->UpdateDisplay(secondsRunning);
		}

		ThreadUtil::SleepFor(seconds(1));
	}

	LOG_INFO_F("Closing Grin++ v{}", GRINPP_VERSION);
}