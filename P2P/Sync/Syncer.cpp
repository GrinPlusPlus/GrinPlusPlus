#include "Syncer.h"
#include "HeaderSyncer.h"
#include "StateSyncer.h"
#include "BlockSyncer.h"
#include "../ConnectionManager.h"

#include <BlockChain/BlockChainServer.h>
#include <P2P/SyncStatus.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/ThreadUtil.h>

Syncer::Syncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer)
	: m_connectionManager(connectionManager), m_blockChainServer(blockChainServer)
{

}

void Syncer::Start()
{
	m_terminate = false;

	if (m_syncThread.joinable())
	{
		m_syncThread.join();
	}

	m_syncThread = std::thread(Thread_Sync, std::ref(*this));
}

void Syncer::Stop()
{
	m_terminate = true;

	if (m_syncThread.joinable())
	{
		m_syncThread.join();
	}
}

void Syncer::Thread_Sync(Syncer& syncer)
{
	ThreadManagerAPI::SetCurrentThreadName("SYNC_THREAD");

	LoggerAPI::LogInfo("Syncer::Thread_Sync() - BEGIN");

	HeaderSyncer headerSyncer(syncer.m_connectionManager, syncer.m_blockChainServer);
	StateSyncer stateSyncer(syncer.m_connectionManager, syncer.m_blockChainServer);
	BlockSyncer blockSyncer(syncer.m_connectionManager, syncer.m_blockChainServer);
	bool startup = true;

	while (!syncer.m_terminate)
	{
		ThreadUtil::SleepFor(std::chrono::milliseconds(50), syncer.m_terminate);
		syncer.UpdateSyncStatus();

		if (syncer.m_syncStatus.GetNumActiveConnections() >= 4)
		{
			// Sync Headers
			if (headerSyncer.SyncHeaders(syncer.m_syncStatus, startup))
			{
				syncer.m_syncStatus.UpdateStatus(ESyncStatus::SYNCING_HEADERS);
				continue;
			}

			// Sync State (TxHashSet)
			if (stateSyncer.SyncState(syncer.m_syncStatus))
			{
				if (syncer.m_syncStatus.GetStatus() != ESyncStatus::PROCESSING_TXHASHSET)
				{
					syncer.m_syncStatus.UpdateStatus(ESyncStatus::SYNCING_TXHASHSET);
				}

				continue;
			}

			// Sync Blocks
			if (blockSyncer.SyncBlocks(syncer.m_syncStatus, startup))
			{
				syncer.m_syncStatus.UpdateStatus(ESyncStatus::SYNCING_BLOCKS);
				continue;
			}

			startup = false;

			syncer.m_syncStatus.UpdateStatus(ESyncStatus::NOT_SYNCING);
		}
		else if (syncer.m_syncStatus.GetStatus() != ESyncStatus::PROCESSING_TXHASHSET)
		{
			syncer.m_syncStatus.UpdateStatus(ESyncStatus::WAITING_FOR_PEERS);
		}
	}

	LoggerAPI::LogInfo("Syncer::Thread_Sync() - END");
}

void Syncer::UpdateSyncStatus()
{
	m_blockChainServer.UpdateSyncStatus(m_syncStatus);
	m_connectionManager.UpdateSyncStatus(m_syncStatus);
}