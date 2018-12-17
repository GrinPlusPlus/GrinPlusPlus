#pragma once

#include "ConnectionFactory.h"

#include <Config/Config.h>
#include <atomic>
#include <thread>

// Forward Declarations
class ConnectionManager;
class PeerManager;
class IBlockChainServer;

class Seeder
{
public:
	Seeder(const Config& config, ConnectionManager& connectionManager, PeerManager& peerManager, IBlockChainServer& blockChainServer);

	void Start();
	void Stop();

private:
	static void Thread_Seed(Seeder& seeder);

	bool SeedNewConnection();
	bool ListenForConnections();

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	PeerManager& m_peerManager;
	ConnectionFactory m_connectionFactory;

	std::atomic<bool> m_terminate = true;
	std::thread m_seedThread;
	mutable bool m_usedDNS = false;
};