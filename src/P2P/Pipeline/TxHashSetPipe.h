#pragma once

#include "../ConnectionManager.h"

#include <P2P/SyncStatus.h>
#include <Crypto/Hash.h>
#include <Net/Socket.h>
#include <BlockChain/BlockChainServer.h>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>

// Forward Declarations
class Config;
class TxHashSetArchiveMessage;

class TxHashSetPipe
{
public:
	static std::shared_ptr<TxHashSetPipe> Create(
		const Config& config,
		ConnectionManagerPtr pConnectionManager,
		IBlockChainServerPtr pBlockChainServer,
		SyncStatusPtr pSyncStatus
	);
	~TxHashSetPipe();

	//
	// Downloads a TxHashSet and kicks off a new thread to process it.
	// Caller should ban peer if false is returned.
	//
	bool ReceiveTxHashSet(const uint64_t connectionId, Socket& socket, const TxHashSetArchiveMessage& txHashSetArchiveMessage);

private:
	TxHashSetPipe(
		const Config& config,
		ConnectionManagerPtr pConnectionManager,
		IBlockChainServerPtr pBlockChainServer,
		SyncStatusPtr pSyncStatus
	);

	const Config& m_config;
	ConnectionManagerPtr m_pConnectionManager;
	IBlockChainServerPtr m_pBlockChainServer;
	SyncStatusPtr m_pSyncStatus;

	static void Thread_ProcessTxHashSet(TxHashSetPipe& pipeline, const uint64_t connectionId, const Hash blockHash, const std::string path);
	std::thread m_txHashSetThread;

	std::atomic_bool m_processing;
};