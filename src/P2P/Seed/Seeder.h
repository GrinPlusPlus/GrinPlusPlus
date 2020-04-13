#pragma once

#include "../ConnectionManager.h"
#include "../MessageProcessor.h"

#include <Config/Config.h>
#include <Core/Context.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/Direction.h>
#include <cstdint>
#include <memory>
#include <atomic>
#include <thread>
#include <optional>
#include <asio.h>

// Forward Declarations
class Connection;
class PeerManager;
class Pipeline;

class Seeder
{
public:
	static std::shared_ptr<Seeder> Create(
		Context::Ptr pContext,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		IBlockChainServerPtr pBlockChainServer,
		std::shared_ptr<Pipeline> pPipeline,
		SyncStatusConstPtr pSyncStatus
	);
	~Seeder();

private:
	Seeder(
		Context::Ptr pContext,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		IBlockChainServerPtr pBlockChainServer,
		std::shared_ptr<MessageProcessor> pMessageProcessor,
		SyncStatusConstPtr pSyncStatus
	);

	static void Thread_Seed(Seeder& seeder);
	static void Thread_Listener(Seeder& seeder);

	ConnectionPtr SeedNewConnection();

	Context::Ptr m_pContext;
	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	IBlockChainServerPtr m_pBlockChainServer;
	std::shared_ptr<MessageProcessor> m_pMessageProcessor;
	SyncStatusConstPtr m_pSyncStatus;

	std::atomic<bool> m_terminate = true;

	std::shared_ptr<asio::io_context> m_pAsioContext;
	std::thread m_seedThread;
	std::thread m_listenerThread;
	mutable std::atomic_bool m_usedDNS = false;
	mutable uint64_t m_nextId = { 1 };
};