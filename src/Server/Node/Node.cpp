#include "Node.h"
#include "NodeRPCServer.h"
#include "NodeClients/DefaultNodeClient.h"
#include "../console.h"

#include <Consensus.h>
#include <Core/Context.h>
#include <P2P/P2PServer.h>
#include <Core/Config.h>
#include <BlockChain/BlockChain.h>
#include <Database/Database.h>
#include <Common/Logger.h>
#include <PMMR/TxHashSetManager.h>

#include <iostream>
#include <thread>

Node::Node(
	const Context::Ptr& pContext,
	std::unique_ptr<NodeRPCServer>&& pNodeRPCServer,
	std::shared_ptr<DefaultNodeClient> pNodeClient)
	: m_pContext(pContext),
	m_pNodeRPCServer(std::move(pNodeRPCServer)),
	m_pNodeClient(pNodeClient)
{

}

Node::~Node()
{
	LOG_INFO("Shutting down node daemon");
}

std::unique_ptr<Node> Node::Create(const Context::Ptr& pContext, const ServerPtr& pServer)
{
	auto pNodeClient = DefaultNodeClient::Create(pContext);
	auto pNodeRPCServer = NodeRPCServer::Create(
		pServer,
		pNodeClient->GetNodeContext()
	);

	return std::make_unique<Node>(
		pContext,
		std::move(pNodeRPCServer),
		pNodeClient
	);
}

std::shared_ptr<INodeClient> Node::GetNodeClient() const
{
	return m_pNodeClient;
}

void Node::UpdateDisplay(const int secondsRunning)
{
	SyncStatusConstPtr pSyncStatus = m_pNodeClient->GetP2PServer()->GetSyncStatus();

	IO::Clear();

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
		const double percentage = networkHeight > 0 ? (pSyncStatus->GetHeaderHeight() * 100 / (networkHeight * 1.0)) : 0;
		std::cout << "\nStatus: Syncing Headers (" << percentage << "%)";
	}
	else if (status == ESyncStatus::SYNCING_TXHASHSET)
	{
		const uint64_t downloaded = pSyncStatus->GetDownloaded();
		const uint64_t downloadSize = pSyncStatus->GetDownloadSize();
		const double percentage = downloadSize > 0 ? (downloaded * 100) / (downloadSize * 1.0) : 0;

		std::cout << "\nStatus: Downloading TxHashSet " << downloaded << "/" << downloadSize << "(" << percentage << "%)";
	}
	else if (status == ESyncStatus::PROCESSING_TXHASHSET)
	{
		std::cout << "\nStatus: Validating TxHashSet (" << (int)pSyncStatus->GetProcessingStatus() << "%)";
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

	IO::Flush();
}