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

NodeDaemon::NodeDaemon(const Config& config, std::shared_ptr<NodeRestServer> pNodeRestServer, std::shared_ptr<DefaultNodeClient> pNodeClient)
	: m_config(config), m_pNodeRestServer(pNodeRestServer), m_pNodeClient(pNodeClient)
{

}

std::shared_ptr<NodeDaemon> NodeDaemon::Create(const Config& config)
{
	std::shared_ptr<DefaultNodeClient> pNodeClient = DefaultNodeClient::Create(config);
	std::shared_ptr<NodeRestServer> pNodeRestServer = NodeRestServer::Create(config, pNodeClient->GetNodeContext());

	return std::make_shared<NodeDaemon>(NodeDaemon(config, pNodeRestServer, pNodeClient));
}

void NodeDaemon::UpdateDisplay(const int secondsRunning)
{
	SyncStatusConstPtr pSyncStatus = m_pNodeClient->GetP2PServer()->GetSyncStatus();

#ifdef _WIN32
		std::system("cls");
#else
		std::system("clear");
#endif

	std::cout << "Time Running: " << secondsRunning << "s";

	const ESyncStatus status = pSyncStatus->GetStatus();
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
		const uint64_t networkHeight = pSyncStatus->GetNetworkHeight();
		const uint64_t percentage = networkHeight > 0 ? (pSyncStatus->GetHeaderHeight() * 100 / networkHeight) : 0;
		std::cout << "\nStatus: Syncing Headers (" << percentage << "%)";
	}
	else if (status == ESyncStatus::SYNCING_TXHASHSET)
	{
		const uint64_t downloaded = pSyncStatus->GetDownloaded();
		const uint64_t downloadSize = pSyncStatus->GetDownloadSize();
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

	std::cout << "\nNumConnections: " << pSyncStatus->GetNumActiveConnections();
	std::cout << "\nHeader Height: " << pSyncStatus->GetHeaderHeight();
	std::cout << "\nHeader Difficulty: " << pSyncStatus->GetHeaderDifficulty();
	std::cout << "\nBlock Height: " << pSyncStatus->GetBlockHeight();
	std::cout << "\nBlock Difficulty: " << pSyncStatus->GetBlockDifficulty();
	std::cout << "\nNetwork Height: " << pSyncStatus->GetNetworkHeight();
	std::cout << "\nNetwork Difficulty: " << pSyncStatus->GetNetworkDifficulty();
	std::cout << "\n\nPress Ctrl-C to exit...";
	std::cout << std::flush;
}