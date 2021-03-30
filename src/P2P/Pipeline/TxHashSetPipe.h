#pragma once

#include <P2P/SyncStatus.h>
#include <Crypto/Models/Hash.h>
#include <Net/Socket.h>
#include <P2P/Peer.h>
#include <BlockChain/BlockChain.h>
#include <Common/Util/FileUtil.h>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>

// Forward Declarations
class Config;
class Connection;
class TxHashSetArchiveMessage;

class TxHashSetPipe
{
public:
	static std::shared_ptr<TxHashSetPipe> Create(
		const Config& config,
		const IBlockChain::Ptr& pBlockChain,
		SyncStatusPtr pSyncStatus
	);

	~TxHashSetPipe();

	//
	// Downloads a TxHashSet and kicks off a new thread to process it.
	// Caller should ban peer if false is returned.
	//
	void ReceiveTxHashSet(Connection& connection, const TxHashSetArchiveMessage& txHashSetArchiveMessage);

private:
	TxHashSetPipe(
		const Config& config,
		const IBlockChain::Ptr& pBlockChain,
		SyncStatusPtr pSyncStatus
	) : m_config(config),
		m_pBlockChain(pBlockChain),
		m_pSyncStatus(pSyncStatus),
		m_processing(false) { }

	const Config& m_config;
	IBlockChain::Ptr m_pBlockChain;
	SyncStatusPtr m_pSyncStatus;

	static void Thread_ProcessTxHashSet(TxHashSetPipe& pipeline, PeerPtr pPeer, const Hash blockHash, const fs::path path);
	std::thread m_txHashSetThread;

	std::atomic_bool m_processing;
};