#pragma once

#include <P2P/Peer.h>
#include <P2P/Direction.h>
#include <Core/Traits/Printable.h>

class ConnectedPeer : public Traits::IPrintable
{
public:
	ConnectedPeer(PeerPtr peer, const EDirection direction)
		: m_pPeer(peer), m_direction(direction), m_height(0), m_totalDifficulty(0)
	{

	}
	ConnectedPeer(const ConnectedPeer& peer)
		: m_pPeer(peer.m_pPeer), m_direction(peer.m_direction), m_height(peer.m_height.load()), m_totalDifficulty(peer.m_totalDifficulty.load())
	{

	}

	virtual ~ConnectedPeer() = default;

	void UpdateTotals(const uint64_t totalDifficulty, const uint64_t height)
	{
		m_totalDifficulty.exchange(totalDifficulty);
		m_height.exchange(height);
	}

	PeerConstPtr GetPeer() const { return m_pPeer; }
	PeerPtr GetPeer() { return m_pPeer; }
	const EDirection GetDirection() const { return m_direction; }
	const uint64_t GetTotalDifficulty() const { return m_totalDifficulty.load(); }
	const uint64_t GetHeight() const { return m_height.load(); }

	void UpdateVersion(const uint32_t version) { m_pPeer->UpdateVersion(version); }
	void UpdateCapabilities(const Capabilities& capabilities) { m_pPeer->UpdateCapabilities(capabilities); }
	void UpdateUserAgent(const std::string& userAgent) { m_pPeer->UpdateUserAgent(userAgent); }
	void UpdateLastContactTime() const { m_pPeer->UpdateLastContactTime(); }

	virtual std::string Format() const override final { return m_pPeer->Format(); }

private:
	EDirection m_direction;
	PeerPtr m_pPeer;
	std::atomic<uint64_t> m_totalDifficulty;
	std::atomic<uint64_t> m_height;
	// TODO: Add Connection Stats
};