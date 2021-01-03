#pragma once

#include "../ConnectionManager.h"
#include "../MessageProcessor.h"

#include <Core/Config.h>
#include <BlockChain/BlockChain.h>
#include <P2P/Direction.h>
#include <cstdint>
#include <memory>
#include <atomic>
#include <thread>
#include <optional>
#include <asio.hpp>

// Forward Declarations
class Context;
class Connection;
class PeerManager;
class Pipeline;

class Seeder
{
public:
	static std::unique_ptr<Seeder> Create(
		const std::shared_ptr<Context>& pContext,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		const IBlockChain::Ptr& pBlockChain,
		std::shared_ptr<Pipeline> pPipeline,
		SyncStatusConstPtr pSyncStatus
	);
	~Seeder();

private:
	Seeder(
		const std::shared_ptr<Context>& pContext,
		ConnectionManager& connectionManager,
		Locked<PeerManager> peerManager,
		const IBlockChain::Ptr& pBlockChain,
		std::shared_ptr<MessageProcessor> pMessageProcessor,
		SyncStatusConstPtr pSyncStatus
	) : m_pContext(pContext),
		m_connectionManager(connectionManager),
		m_peerManager(peerManager),
		m_pBlockChain(pBlockChain),
		m_pMessageProcessor(pMessageProcessor),
		m_pSyncStatus(pSyncStatus),
		m_pAsioContext(std::make_shared<asio::io_context>()),
		m_terminate(false) { }

	static void Thread_Seed(Seeder& seeder);
	static void Thread_Listener(Seeder& seeder);

	ConnectionPtr SeedNewConnection();

	std::shared_ptr<Context> m_pContext;
	ConnectionManager& m_connectionManager;
	Locked<PeerManager> m_peerManager;
	IBlockChain::Ptr m_pBlockChain;
	std::shared_ptr<MessageProcessor> m_pMessageProcessor;
	SyncStatusConstPtr m_pSyncStatus;

	std::atomic<bool> m_terminate = true;

	std::shared_ptr<asio::io_context> m_pAsioContext;
	std::thread m_seedThread;
	std::thread m_listenerThread;
	mutable std::atomic_bool m_usedDNS = false;
	mutable uint64_t m_nextId = { 1 };
};