#pragma once

#include <P2P/SyncStatus.h>
#include <atomic>
#include <thread>

// Forward Declarations
class ConnectionManager;
class IBlockChainServer;
class SyncStatus;

class Syncer
{
public:
	Syncer(ConnectionManager& connectionManager, IBlockChainServer& blockChainServer);

	void Start();
	void Stop();

	inline SyncStatus& GetSyncStatus() { return m_syncStatus; }
	inline const SyncStatus& GetSyncStatus() const { return m_syncStatus; }

private:
	static void Thread_Sync(Syncer& syncer);
	void UpdateSyncStatus();

	ConnectionManager& m_connectionManager;
	IBlockChainServer& m_blockChainServer;
	SyncStatus m_syncStatus;

	std::atomic<bool> m_terminate;
	std::thread m_syncThread;
};