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
class ConnectionManager;
class Connection;
class TxHashSetArchiveMessage;

class TxHashSetPipe
{
public:
	static std::shared_ptr<TxHashSetPipe> Create(
		const std::shared_ptr<ConnectionManager>& pConnectionManager,
		const IBlockChain::Ptr& pBlockChain,
		SyncStatusPtr pSyncStatus
	);

	~TxHashSetPipe();

	//
	// Downloads a TxHashSet and kicks off a new thread to process it.
	//
	void ReceiveTxHashSet(
		const std::shared_ptr<Connection>& pConnection,
		const TxHashSetArchiveMessage& archive_msg
	);

	void SendTxHashSet(
		const std::shared_ptr<Connection>& pConnection,
		const Hash& block_hash
	);

private:
	TxHashSetPipe(
		const std::shared_ptr<ConnectionManager>& pConnectionManager,
		const IBlockChain::Ptr& pBlockChain,
		SyncStatusPtr pSyncStatus
	) : m_pConnectionManager(pConnectionManager),
		m_pBlockChain(pBlockChain),
		m_pSyncStatus(pSyncStatus),
		m_processing(false) { }

	std::shared_ptr<ConnectionManager> m_pConnectionManager;
	IBlockChain::Ptr m_pBlockChain;
	SyncStatusPtr m_pSyncStatus;

	static void Thread_ProcessTxHashSet(
		TxHashSetPipe& pipeline,
		std::shared_ptr<Connection> pConnection,
		const uint64_t zipped_size,
		const Hash blockHash
	);

	static void Thread_SendTxHashSet(
		IBlockChain::Ptr pBlockChain,
		std::shared_ptr<Connection> pConnection,
		Hash block_hash
	);

	std::thread m_txHashSetThread;
	std::vector<std::thread> m_sendThreads;

	std::atomic_bool m_processing;
};