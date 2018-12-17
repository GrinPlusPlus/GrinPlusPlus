#pragma once

#include <stdint.h>
#include <atomic>

class SyncStatus
{
public:
	SyncStatus()
		: m_headerHeight(0),
		m_headerDifficulty(0),
		m_blockHeight(0),
		m_blockDifficulty(0),
		m_txHashSetDownloaded(0),
		m_txHashSetTotalSize(0)
	{

	}
	SyncStatus(const SyncStatus& other)
		: m_headerHeight(other.m_headerHeight.load()),
		m_headerDifficulty(other.m_headerDifficulty.load()),
		m_blockHeight(other.m_blockHeight.load()),
		m_blockDifficulty(other.m_blockDifficulty.load()),
		m_txHashSetDownloaded(other.m_txHashSetDownloaded.load()),
		m_txHashSetTotalSize(other.m_txHashSetTotalSize.load())
	{

	}

	inline uint64_t GetHeaderHeight() const { return m_headerHeight; }
	inline uint64_t GetHeaderDifficulty() const { return m_headerDifficulty; }
	inline uint64_t GetBlockHeight() const { return m_blockHeight; }
	inline uint64_t GetBlockDifficulty() const { return m_blockDifficulty; }
	inline uint64_t GetDownloaded() const { return m_txHashSetDownloaded; }
	inline uint64_t GetDownloadSize() const { return m_txHashSetTotalSize; }

	void UpdateHeaderStatus(const uint64_t headerHeight, const uint64_t headerDifficulty)
	{
		m_headerHeight = headerHeight;
		m_headerDifficulty = headerDifficulty;
	}

	void UpdateBlockStatus(const uint64_t blockHeight, const uint64_t blockDifficulty)
	{
		m_blockHeight = blockHeight;
		m_blockDifficulty = blockDifficulty;
	}

	inline void UpdateDownloaded(const uint64_t downloaded) { m_txHashSetDownloaded = downloaded; }
	inline void UpdateDownloadSize(const uint64_t downloadSize) { m_txHashSetTotalSize = downloadSize; }

private:
	std::atomic<uint64_t> m_headerHeight;
	std::atomic<uint64_t> m_headerDifficulty;
	std::atomic<uint64_t> m_blockHeight;
	std::atomic<uint64_t> m_blockDifficulty;
	std::atomic<uint64_t> m_txHashSetDownloaded;
	std::atomic<uint64_t> m_txHashSetTotalSize;
};