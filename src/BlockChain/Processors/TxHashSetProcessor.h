#pragma once

#include "../ChainState.h"

#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <Crypto/Hash.h>
#include <P2P/SyncStatus.h>
#include <string>

// Forward Declarations
class IBlockChainServer;
class BlockHeader;
class IBlockDB;

class TxHashSetProcessor
{
public:
	TxHashSetProcessor(const Config& config, IBlockChainServer& blockChainServer, std::shared_ptr<Locked<ChainState>> pChainState, std::shared_ptr<Locked<IBlockDB>> pBlockDB);

	bool ProcessTxHashSet(const Hash& blockHash, const std::string& path, SyncStatus& syncStatus);

private:
	bool UpdateConfirmedChain(Writer<ChainState> pLockedState, const BlockHeader& blockHeader);

	const Config& m_config;
	IBlockChainServer& m_blockChainServer;
	std::shared_ptr<Locked<ChainState>> m_pChainState;
	std::shared_ptr<Locked<IBlockDB>> m_pBlockDB;
};