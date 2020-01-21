#pragma once

#include <P2P/SyncStatus.h>
#include <Crypto/Hash.h>
#include <Net/Socket.h>
#include <P2P/Peer.h>
#include <BlockChain/BlockChainServer.h>
#include <Common/Util/FileUtil.h>
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
		IBlockChainServerPtr pBlockChainServer,
		SyncStatusPtr pSyncStatus
	);
	~TxHashSetPipe();

	//
	// Downloads a TxHashSet and kicks off a new thread to process it.
	// Caller should ban peer if false is returned.
	//
	bool ReceiveTxHashSet(PeerPtr pPeer, Socket& socket, const TxHashSetArchiveMessage& txHashSetArchiveMessage);

private:
	TxHashSetPipe(
		const Config& config,
		IBlockChainServerPtr pBlockChainServer,
		SyncStatusPtr pSyncStatus
	);

	const Config& m_config;
	IBlockChainServerPtr m_pBlockChainServer;
	SyncStatusPtr m_pSyncStatus;

	static void Thread_ProcessTxHashSet(TxHashSetPipe& pipeline, PeerPtr pPeer, const Hash blockHash, const fs::path path);
	std::thread m_txHashSetThread;

	std::atomic_bool m_processing;
};