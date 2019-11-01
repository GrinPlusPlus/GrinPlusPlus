#pragma once

#include <Config/Config.h>
#include <BlockChain/BlockChainServer.h>
#include <TxPool/TransactionPool.h>

#include <thread>
#include <atomic>
#include <chrono>

// Forward Declarations
class ConnectionManager;

class Dandelion
{
public:
	Dandelion(
		const Config& config,
		ConnectionManager& connectionManager,
		IBlockChainServerPtr pBlockChainServer,
		ITransactionPoolPtr pTransactionPool,
		std::shared_ptr<const Locked<IBlockDB>> pBlockDB
	);

	void Start();
	void Stop();

	static void Thread_Monitor(Dandelion& dandelion);

private:
	bool ProcessStemPhase();
	bool ProcessFluffPhase();
	bool ProcessExpiredEntries();

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	IBlockChainServerPtr m_pBlockChainServer;
	ITransactionPoolPtr m_pTransactionPool;
	std::shared_ptr<const Locked<IBlockDB>> m_pBlockDB;

	std::atomic_bool m_terminate = true;
	std::thread m_dandelionThread;

	uint64_t m_relayNodeId;
	std::chrono::time_point<std::chrono::system_clock> m_relayExpirationTime;
};