#pragma once

#include <cstdint>
#include <memory>
#include <atomic>

enum class ESyncStatus
{
	SYNCING_HEADERS,
	SYNCING_TXHASHSET,
	PROCESSING_TXHASHSET,
	TXHASHSET_SYNC_FAILED,
	SYNCING_BLOCKS,
	WAITING_FOR_PEERS,
	NOT_SYNCING
};

class SyncStatus
{
public:
	SyncStatus()
		: m_syncStatus(ESyncStatus::SYNCING_HEADERS),
		m_numActiveConnections(0),
		m_networkHeight(0),
		m_networkDifficulty(0),
		m_headerHeight(0),
		m_headerDifficulty(0),
		m_blockHeight(0),
		m_blockDifficulty(0),
		m_lastBlockTime(0),
		m_txHashSetDownloaded(0),
		m_txHashSetTotalSize(0),
		m_txHashSetProcessingStatus(0)
	{

	}
	SyncStatus(const SyncStatus& other)
		: m_syncStatus(other.m_syncStatus.load()),
		m_numActiveConnections(other.m_numActiveConnections.load()),
		m_networkHeight(other.m_networkHeight.load()),
		m_networkDifficulty(other.m_networkDifficulty.load()),
		m_headerHeight(other.m_headerHeight.load()),
		m_headerDifficulty(other.m_headerDifficulty.load()),
		m_blockHeight(other.m_blockHeight.load()),
		m_blockDifficulty(other.m_blockDifficulty.load()),
		m_lastBlockTime(other.m_lastBlockTime.load()),
		m_txHashSetDownloaded(other.m_txHashSetDownloaded.load()),
		m_txHashSetTotalSize(other.m_txHashSetTotalSize.load()),
		m_txHashSetProcessingStatus(other.m_txHashSetProcessingStatus.load())
	{

	}

	bool IsSyncing() const { return m_syncStatus != ESyncStatus::NOT_SYNCING; }
	ESyncStatus GetStatus() const { return m_syncStatus; }
	uint64_t GetNumActiveConnections() const { return m_numActiveConnections; }
	uint64_t GetNetworkHeight() const { return m_networkHeight; }
	uint64_t GetNetworkDifficulty() const { return m_networkDifficulty; }
	uint64_t GetHeaderHeight() const { return m_headerHeight; }
	uint64_t GetHeaderDifficulty() const { return m_headerDifficulty; }
	uint64_t GetBlockHeight() const { return m_blockHeight; }
	uint64_t GetBlockDifficulty() const { return m_blockDifficulty; }
	time_t GetLastBlockTime() const noexcept { return m_lastBlockTime; }
	uint64_t GetDownloaded() const { return m_txHashSetDownloaded; }
	uint64_t GetDownloadSize() const { return m_txHashSetTotalSize; }
	uint8_t GetProcessingStatus() const { return m_txHashSetProcessingStatus; }

	void UpdateStatus(const ESyncStatus syncStatus) { m_syncStatus = syncStatus; }

	void UpdateNetworkStatus(const uint64_t numActiveConnections, const uint64_t networkHeight, const uint64_t networkDifficulty)
	{
		m_numActiveConnections = numActiveConnections;
		m_networkHeight = networkHeight;
		m_networkDifficulty = networkDifficulty;
	}

	void UpdateHeaderStatus(const uint64_t headerHeight, const uint64_t headerDifficulty)
	{
		m_headerHeight = headerHeight;
		m_headerDifficulty = headerDifficulty;
	}

	void UpdateBlockStatus(const uint64_t blockHeight, const uint64_t blockDifficulty, const time_t lastBlockTime)
	{
		m_blockHeight = blockHeight;
		m_blockDifficulty = blockDifficulty;
		m_lastBlockTime = lastBlockTime;
	}

	void UpdateDownloaded(const uint64_t downloaded) { m_txHashSetDownloaded = downloaded; }
	void UpdateDownloadSize(const uint64_t downloadSize) { m_txHashSetTotalSize = downloadSize; }
	void UpdateProcessingStatus(const uint8_t processingStatus) { m_txHashSetProcessingStatus = processingStatus; }

private:
	std::atomic<ESyncStatus> m_syncStatus;
	std::atomic<uint64_t> m_numActiveConnections;
	std::atomic<uint64_t> m_networkHeight;
	std::atomic<uint64_t> m_networkDifficulty;
	std::atomic<uint64_t> m_headerHeight;
	std::atomic<uint64_t> m_headerDifficulty;
	std::atomic<uint64_t> m_blockHeight;
	std::atomic<uint64_t> m_blockDifficulty;
	std::atomic<time_t> m_lastBlockTime;
	std::atomic<uint64_t> m_txHashSetDownloaded;
	std::atomic<uint64_t> m_txHashSetTotalSize;
	std::atomic<uint8_t> m_txHashSetProcessingStatus;
};

typedef std::shared_ptr<SyncStatus> SyncStatusPtr;
typedef std::shared_ptr<SyncStatus> SyncStatusConstPtr;