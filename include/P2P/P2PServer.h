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
	virtual ~IP2PServer() {}

	virtual const SyncStatus& GetSyncStatus() const = 0;

	//
	// Returns the number of peers the client is currently connected to (inbound, outbound).
	//
	virtual std::pair<size_t, size_t> GetNumberOfConnectedPeers() const = 0;

	virtual std::vector<Peer> GetAllPeers() const = 0;

	virtual std::vector<ConnectedPeer> GetConnectedPeers() const = 0;


	virtual std::optional<Peer> GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const = 0;

	virtual bool BanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt, const EBanReason banReason) = 0;

	virtual bool UnbanPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) = 0;

	virtual bool UnbanAllPeers() = 0;
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