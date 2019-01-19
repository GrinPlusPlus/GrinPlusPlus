#include "Syncer.h"
#include "HeaderSyncer.h"
#include "StateSyncer.h"
#include "BlockSyncer.h"
#include "../ConnectionManager.h"

#include <BlockChainServer.h>
#include <P2P/SyncStatus.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>

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

	while (!syncer.m_terminate)
	{
		syncer.UpdateSyncStatus();

		if (syncer.m_connectionManager.GetNumberOfActiveConnections() > 2)
		{
			// Sync Headers
			if (headerSyncer.SyncHeaders(syncer.m_syncStatus))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(50));
				continue;
			}

			// Sync State (TxHashSet)
			if (stateSyncer.SyncState(syncer.m_syncStatus))
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				continue;
			}

			// Sync Blocks
			if (blockSyncer.SyncBlocks(syncer.m_syncStatus))
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
		}

		std::this_thread::sleep_for(std::chrono::seconds(2));
	}

	LoggerAPI::LogInfo("Syncer::Thread_Sync() - END");
}

void Syncer::UpdateSyncStatus()
{
	m_blockChainServer.UpdateSyncStatus(m_syncStatus);
	m_syncStatus.UpdateDownloaded(0);
	m_syncStatus.UpdateDownloadSize(0);
}