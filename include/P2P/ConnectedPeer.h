#pragma once

#include <P2P/Peer.h>
#include <P2P/Direction.h>
#include <Core/Traits/Printable.h>

class ConnectedPeer : public Traits::IPrintable
{
public:
	ConnectedPeer(PeerPtr peer, const EDirection direction, const uint16_t portNumber)
		: m_pPeer(peer), m_direction(direction), m_portNumber(portNumber), m_totalDifficulty(0), m_height(0)
	{

	}
	ConnectedPeer(const ConnectedPeer& peer)
		: m_pPeer(peer.m_pPeer), m_direction(peer.m_direction), m_portNumber(peer.m_portNumber), m_totalDifficulty(peer.m_totalDifficulty.load()), m_height(peer.m_height.load())
	{

	}

	virtual ~ConnectedPeer() = default;

	void UpdateTotals(const uint64_t totalDifficulty, const uint64_t height)
	{
		m_totalDifficulty.exchange(totalDifficulty);
		m_height.exchange(height);
	}

	PeerConstPtr GetPeer() const noexcept { return m_pPeer; }
	PeerPtr GetPeer() noexcept { return m_pPeer; }
	SocketAddress GetSocketAddress() const noexcept { return SocketAddress(m_pPeer->GetIPAddress(), m_portNumber); }
	const IPAddress& GetIPAddress() const noexcept { return m_pPeer->GetIPAddress(); }
	const EDirection GetDirection() const noexcept { return m_direction; }
	uint16_t GetPort() const noexcept { return m_portNumber; }
	uint64_t GetTotalDifficulty() const noexcept { return m_totalDifficulty.load(); }
	uint64_t GetHeight() const noexcept { return m_height.load(); }
	uint32_t GetProtocolVersion() const noexcept { return m_pPeer->GetVersion(); }

	void UpdateVersion(const uint32_t version) { m_pPeer->UpdateVersion(version); }
	void UpdateCapabilities(const Capabilities& capabilities) { m_pPeer->UpdateCapabilities(capabilities); }
	void UpdateUserAgent(const std::string& userAgent) { m_pPeer->UpdateUserAgent(userAgent); }
	void UpdateLastContactTime() const { m_pPeer->UpdateLastContactTime(); }

	Json::Value ToJSON() const
	{
		Json::Value json = GetPeer()->ToJSON();
		json["direction"] = GetDirection() == EDirection::OUTBOUND ? "Outbound" : "Inbound";
		json["total_difficulty"] = GetTotalDifficulty();
		json["height"] = GetHeight();
		return json;
	}

	std::string Format() const final { return m_pPeer->Format(); }

private:
	EDirection m_direction;
	PeerPtr m_pPeer;
	uint16_t m_portNumber;
	std::atomic<uint64_t> m_totalDifficulty;
	std::atomic<uint64_t> m_height;
	// TODO: Add Connection Stats
};