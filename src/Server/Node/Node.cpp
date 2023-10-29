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

	std::cout << "\nTime Running: \t\t" << secondsRunning << "s";

	const ESyncStatus status = pSyncStatus->GetStatus();
	if (status == ESyncStatus::NOT_SYNCING)
	{
		std::cout << "\nStatus: \t\tRUNNING";
	}
	else if (status == ESyncStatus::WAITING_FOR_PEERS)
	{
		std::cout << "\nStatus: \t\tWAITING FOR PEERS";
	}
	else if (status == ESyncStatus::SYNCING_HEADERS)
	{
		const uint64_t networkHeight = pSyncStatus->GetNetworkHeight();
		const double percentage = networkHeight > 0 ? (pSyncStatus->GetHeaderHeight() * 100 / (networkHeight * 1.0)) : 0;
		std::cout << "\nStatus: \t\tSYNCING HEADERS (" << percentage << "%)";
	}
	else if (status == ESyncStatus::SYNCING_TXHASHSET)
	{
		const uint64_t downloaded = pSyncStatus->GetDownloaded();
		const uint64_t downloadSize = pSyncStatus->GetDownloadSize();
		const double percentage = downloadSize > 0 ? (downloaded * 100) / (downloadSize * 1.0) : 0;

		std::cout << "\nStatus: \t\tDOWNLOADING TxHashSet " << downloaded << "/" << downloadSize << "(" << percentage << "%)";
	}
	else if (status == ESyncStatus::PROCESSING_TXHASHSET)
	{
		std::cout << "\nStatus: \t\tVALIDATING TxHashSet (" << (int)pSyncStatus->GetProcessingStatus() << "%)";
	}
	else if (status == ESyncStatus::SYNCING_BLOCKS)
	{
		std::cout << "\nStatus: \t\tSYNCING BLOCKS";
	}

	std::cout << "\nWorkeable Peers: \t" << pSyncStatus->GetNumActiveConnections();
	std::cout << "\nHeaders Height: \t" << pSyncStatus->GetHeaderHeight();
	std::cout << "\nBlocks Height: \t\t" << pSyncStatus->GetBlockHeight();
	std::cout << "\nNetwork Height: \t" << pSyncStatus->GetNetworkHeight();
	std::cout << "\nHeaders Difficulty: \t" << pSyncStatus->GetHeaderDifficulty();
	std::cout << "\nBlocks Difficulty: \t" << pSyncStatus->GetBlockDifficulty();
	std::cout << "\nNetwork Difficulty: \t" << pSyncStatus->GetNetworkDifficulty();
	std::cout << "\n\nPress Ctrl-C to exit...";

	IO::Flush();
}