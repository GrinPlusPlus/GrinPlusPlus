#pragma once

#include <P2P/peer.h>
#include <P2P/Direction.h>

typedef unsigned __int64 SOCKET;

class ConnectedPeer
{
public:
	ConnectedPeer(const SOCKET socket,  const Peer& peer, const EDirection direction)
		: m_socket(socket), m_peer(peer), m_direction(direction), m_height(0), m_totalDifficulty(0)
	{

	}
	ConnectedPeer(const ConnectedPeer& peer)
		: m_socket(peer.m_socket), m_peer(peer.m_peer), m_direction(peer.m_direction), m_height(peer.m_height.load()), m_totalDifficulty(peer.m_totalDifficulty.load())
	{

	}

	inline void UpdateTotals(const uint64_t totalDifficulty, const uint64_t height)
	{
		m_totalDifficulty.exchange(totalDifficulty);
		m_height.exchange(height);
	}

	inline const SOCKET GetSocket() const { return m_socket; }
	inline const Peer& GetPeer() const { return m_peer; }
	inline Peer& GetPeer() { return m_peer; }
	inline const EDirection GetDirection() const { return m_direction; }
	inline const uint64_t GetTotalDifficulty() const { return m_totalDifficulty.load(); }
	inline const uint64_t GetHeight() const { return m_height.load(); }

	inline void UpdateVersion(const uint32_t version) { m_peer.UpdateVersion(version); }
	inline void UpdateCapabilities(const Capabilities& capabilities) { m_peer.UpdateCapabilities(capabilities); }
	inline void UpdateUserAgent(const std::string& userAgent) { m_peer.UpdateUserAgent(userAgent); }
	inline void UpdateLastContactTime() const { m_peer.UpdateLastContactTime(); }


private:
	SOCKET m_socket; // TODO: Socket belongs in Connection
	EDirection m_direction;
	Peer m_peer;
	std::atomic<uint64_t> m_totalDifficulty;
	std::atomic<uint64_t> m_height;
	// TODO: Add Connection Stats
};