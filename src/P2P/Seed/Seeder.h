#pragma once

#include "../ConnectionManager.h"

#include <Config/Config.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/Direction.h>
#include <memory>
#include <atomic>
#include <thread>
#include <optional>
#include <asio.hpp>

// Forward Declarations
class Connection;
class PeerManager;
class Pipeline;

class Seeder
{
public:
	static std::shared_ptr<Seeder> Create(
		const Config& config,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		IBlockChainServerPtr pBlockChainServer,
		std::shared_ptr<Pipeline> pPipeline,
		SyncStatusConstPtr pSyncStatus
	);
	~Seeder();

private:
	Seeder(
		const Config& config,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		IBlockChainServerPtr pBlockChainServer,
		std::shared_ptr<Pipeline> pPipeline,
		SyncStatusConstPtr pSyncStatus
	);

	static void Thread_Seed(Seeder& seeder);
	static void Thread_Listener(Seeder& seeder);

	ConnectionPtr SeedNewConnection();

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	IBlockChainServerPtr m_pBlockChainServer;
	std::shared_ptr<Pipeline> m_pPipeline;
	SyncStatusConstPtr m_pSyncStatus;

	std::atomic<bool> m_terminate = true;

	std::shared_ptr<asio::io_context> m_pContext;
	std::thread m_seedThread;
	std::thread m_listenerThread;
	mutable std::atomic_bool m_usedDNS = false;
	mutable uint64_t m_nextId = { 1 };
};