#pragma once

#include <Config/Config.h>
#include <BlockChain/BlockChainServer.h>
#include <TxPool/TransactionPool.h>
#include <P2P/Peer.h>

#include <thread>
#include <atomic>
#include <chrono>

// Forward Declarations
class ConnectionManager;

class Dandelion
{
public:
	static std::shared_ptr<Dandelion> Create(
		const Config& config,
		ConnectionManager& connectionManager,
		IBlockChainServerPtr pBlockChainServer,
		std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
		ITransactionPoolPtr pTransactionPool,
		std::shared_ptr<const Locked<IBlockDB>> pBlockDB
	);
	~Dandelion();

private:
	Dandelion(
		const Config& config,
		ConnectionManager& connectionManager,
		IBlockChainServerPtr pBlockChainServer,
		std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
		ITransactionPoolPtr pTransactionPool,
		std::shared_ptr<const Locked<IBlockDB>> pBlockDB
	);

	static void Thread_Monitor(Dandelion& dandelion);

	bool ProcessStemPhase();
	bool ProcessFluffPhase();
	bool ProcessExpiredEntries();

	const Config& m_config;
	ConnectionManager& m_connectionManager;
	IBlockChainServerPtr m_pBlockChainServer;
	std::shared_ptr<Locked<TxHashSetManager>> m_pTxHashSetManager;
	ITransactionPoolPtr m_pTransactionPool;
	std::shared_ptr<const Locked<IBlockDB>> m_pBlockDB;

	std::atomic_bool m_terminate = true;
	std::thread m_dandelionThread;

	PeerPtr m_relayPeer;
	std::chrono::time_point<std::chrono::system_clock> m_relayExpirationTime;
};