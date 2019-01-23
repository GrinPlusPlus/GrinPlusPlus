#pragma once

#include <P2P/peer.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>

class ConnectedPeer
{
public:
	ConnectedPeer(const SOCKET connection, const Peer& peer)
		: m_connection(connection), m_peer(peer), m_height(0), m_totalDifficulty(0)
	{

	}
	ConnectedPeer(const ConnectedPeer& peer)
		: m_connection(peer.m_connection), m_peer(peer.m_peer), m_height(peer.m_height.load()), m_totalDifficulty(peer.m_totalDifficulty.load())
	{

	}

	inline void UpdateTotals(const uint64_t totalDifficulty, const uint64_t height)
	{
		m_totalDifficulty.exchange(totalDifficulty);
		m_height.exchange(height);
	}

	inline const SOCKET GetConnection() const { return m_connection; }
	inline const Peer& GetPeer() const { return m_peer; }
	inline const uint64_t GetTotalDifficulty() const { return m_totalDifficulty.load(); }
	inline const uint64_t GetHeight() const { return m_height.load(); }

private:
	const SOCKET m_connection;
	const Peer m_peer;
	std::atomic<uint64_t> m_totalDifficulty;
	std::atomic<uint64_t> m_height;
	// TODO: Add Connection Stats
};