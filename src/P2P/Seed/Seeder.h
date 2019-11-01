#pragma once

#include <Config/Config.h>
#include <BlockChain/BlockChainServer.h>
#include <P2P/Direction.h>
#include <atomic>
#include <thread>
#include <optional>
#include <asio.hpp>

// Forward Declarations
class Connection;
class ConnectionManager;
class PeerManager;

class Seeder
{
public:
	Seeder(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServerPtr pBlockChainServer);

	void Start();
	void Stop();

private:
	static void Thread_Seed(Seeder& seeder);
	static void Thread_Listener(Seeder& seeder);

	Connection* SeedNewConnection();

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	PeerManager& m_peerManager;
	IBlockChainServerPtr m_pBlockChainServer;

	std::atomic<bool> m_terminate = true;

	asio::io_context m_context;
	std::thread m_seedThread;
	std::thread m_listenerThread;
	mutable std::atomic_bool m_usedDNS = false;
	mutable uint64_t m_nextId = { 1 };
};