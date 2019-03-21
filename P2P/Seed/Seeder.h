#pragma once

#include "ConnectionFactory.h"

#include <Config/Config.h>
#include <P2P/Direction.h>
#include <Net/Listener.h>
#include <atomic>
#include <thread>
#include <optional>

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
	static void Thread_Listener(Seeder& seeder);

	bool SeedNewConnection();
	bool ListenForConnections(const Listener& listenerSocket);
	bool ConnectToPeer(Peer& peer, const EDirection& direction, const std::optional<Socket>& socketOpt);

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	PeerManager& m_peerManager;
	ConnectionFactory m_connectionFactory;

	std::atomic<bool> m_terminate = true;

	std::thread m_seedThread;
	std::thread m_listenerThread;
	mutable std::atomic_bool m_usedDNS = false;
};