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
			if (headerSyncer.SyncHeaders())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}

			// Sync State (TxHashSet)
			if (stateSyncer.SyncState())
			{
				std::this_thread::sleep_for(std::chrono::seconds(1));
				continue;
			}

			// Sync Blocks
			if (blockSyncer.SyncBlocks())
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(100));
				continue;
			}
		}

		std::this_thread::sleep_for(std::chrono::seconds(5));
	}

	LoggerAPI::LogInfo("Syncer::Thread_Sync() - END");
}

// TODO: This is not the best way to handle this.
void Syncer::UpdateSyncStatus()
{
	const uint64_t headerHeight = m_blockChainServer.GetHeight(EChainType::CANDIDATE);
	const uint64_t headerDifficulty = m_blockChainServer.GetTotalDifficulty(EChainType::CANDIDATE);
	m_syncStatus.UpdateHeaderStatus(headerHeight, headerDifficulty);

	const uint64_t blockHeight = m_blockChainServer.GetHeight(EChainType::CONFIRMED);
	const uint64_t blockDifficulty = m_blockChainServer.GetTotalDifficulty(EChainType::CONFIRMED);
	m_syncStatus.UpdateBlockStatus(blockHeight, blockDifficulty);

	m_syncStatus.UpdateDownloaded(0);
	m_syncStatus.UpdateDownloadSize(0);
}