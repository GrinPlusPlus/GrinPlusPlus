#pragma once

#include <Crypto/Hash.h>
#include <Net/Socket.h>
#include <BlockChain/BlockChainServer.h>
#include <string>
#include <cstdint>
#include <atomic>
#include <thread>

// Forward Declarations
class Config;
class ConnectionManager;
class TxHashSetArchiveMessage;

class TxHashSetPipe
{
public:
	TxHashSetPipe(const Config& config, ConnectionManager& connectionManager, IBlockChainServerPtr pBlockChainServer);

	void Stop();

	//
	// Downloads a TxHashSet and kicks off a new thread to process it.
	// Caller should ban peer if false is returned.
	//
	bool ReceiveTxHashSet(const uint64_t connectionId, Socket& socket, const TxHashSetArchiveMessage& txHashSetArchiveMessage);

private:
	const Config& m_config;
	ConnectionManager& m_connectionManager;
	IBlockChainServerPtr m_pBlockChainServer;

	static void Thread_ProcessTxHashSet(TxHashSetPipe& pipeline, const uint64_t connectionId, const Hash blockHash, const std::string path);
	std::thread m_txHashSetThread;

	std::atomic_bool m_processing;
};