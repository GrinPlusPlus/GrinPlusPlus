#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <Crypto/BigInteger.h>
#include <Core/Models/BlockHeader.h>
#include <P2P/SyncStatus.h>
#include <P2P/Peer.h>
#include <P2P/ConnectedPeer.h>
#include <TxPool/TransactionPool.h>
#include <Database/Database.h>
#include <optional>

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
	virtual ~IP2PServer() = default;

	virtual SyncStatusConstPtr GetSyncStatus() const = 0;

	//
	// Returns the number of peers the client is currently connected to (inbound, outbound).
	//
	virtual std::pair<size_t, size_t> GetNumberOfConnectedPeers() const = 0;

	virtual std::vector<PeerConstPtr> GetAllPeers() const = 0;

	virtual std::vector<ConnectedPeer> GetConnectedPeers() const = 0;

	virtual std::optional<PeerConstPtr> GetPeer(
		const IPAddress& address
	) const = 0;

	virtual bool BanPeer(
		const IPAddress& address,
		const EBanReason banReason
	) = 0;

	virtual void UnbanPeer(
		const IPAddress& address
	) = 0;

	virtual bool UnbanAllPeers() = 0;
};

typedef std::shared_ptr<IP2PServer> IP2PServerPtr;

namespace P2PAPI
{
	//
	// Creates a new instance of the P2P Server.
	//
	P2P_API IP2PServerPtr StartP2PServer(
		const Config& config,
		IBlockChainServerPtr pBlockChainServer,
		TxHashSetManagerConstPtr pTxHashSetManager,
		IDatabasePtr pDatabase,
		ITransactionPoolPtr pTransactionPool
	);
}