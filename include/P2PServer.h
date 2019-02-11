#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <BigInteger.h>
#include <Core/BlockHeader.h>
#include <P2P/SyncStatus.h>
#include <TxPool/TransactionPool.h>

#ifdef MW_P2P
#define P2P_API EXPORT
#else
#define P2P_API IMPORT
#endif

// Forward Declarations
class Config;
class IBlockChainServer;
class IDatabase;

//
// This interface acts as the single entry-point into the P2P shared library.
// This handles all interaction with network nodes/peers.
//
class IP2PServer
{
public:
	virtual ~IP2PServer() {}

	//
	// Returns the number of peers the client is currently connected to.
	//
	virtual size_t GetNumberOfConnectedPeers() const = 0;

	virtual const SyncStatus& GetSyncStatus() const = 0;
};

namespace P2PAPI
{
	//
	// Creates a new instance of the P2P Server.
	//
	P2P_API IP2PServer* StartP2PServer(const Config& config, IBlockChainServer& blockChainServer, IDatabase& database, ITransactionPool& transactionPool);

	//
	// Stops the P2P Server and clears up its memory usage.
	//
	P2P_API void ShutdownP2PServer(IP2PServer* pP2PServer);
}