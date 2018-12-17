#pragma once

class P2PConfig
{
public:
	P2PConfig() : P2PConfig(15, 5)
	{
		
	}

	P2PConfig(const int maxPeerConnections, const int preferredMinimumConnections)
		: m_maxPeerConnections(maxPeerConnections), m_preferredMinimumConnections(preferredMinimumConnections)
	{

	}

	inline int GetMaxConnections() const { return m_maxPeerConnections; }
	inline int GetPreferredMinConnections() const { return m_preferredMinimumConnections; }

private:
	int m_maxPeerConnections;
	int m_preferredMinimumConnections;
};