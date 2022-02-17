#pragma once

#include "ConfigProps.h"

#include <cstdint>
#include <json/json.h>
#include <Common/Logger.h>
#include <Core/Enums/Environment.h>

#include <Core/Exceptions/FileException.h>

class P2PConfig
{
public:
	int GetMaxConnections() const noexcept { return m_maxConnections; }
	void SetMaxConnections(const int max_connections) noexcept { m_maxConnections = max_connections; }

	int GetMinConnections() const noexcept { return m_minConnections; }
	void SetMinConnections(const int min_connections) noexcept { m_minConnections = min_connections; }

	uint16_t GetP2PPort() const noexcept { return m_port; }
	const std::vector<uint8_t>& GetMagicBytes() const noexcept { return m_magicBytes; }

	uint8_t GetMinSyncPeers() const noexcept { return m_minSyncPeers; }

    const std::unordered_set<IPAddress>& GetPreferredPeers() const noexcept { return m_peferredPeers; }
	const std::unordered_set<IPAddress>& GetAllowedPeers() const noexcept { return m_allowedPeers; }
	const std::unordered_set<IPAddress>& GetBlockedPeers() const noexcept { return m_blockedPeers; }
	
	//
	// Constructor
	//
	P2PConfig(const Environment env, const Json::Value& json)
	{
		if (env == Environment::MAINNET)  {
			m_port = 3414;
			m_magicBytes = { 97, 61 };
		} else {
			m_port = 13414;
			m_magicBytes = { 83, 59 };
		}

		m_maxConnections = 60;
		m_minConnections = 10;

		if (json.isMember(ConfigProps::P2P::P2P)) {
			const Json::Value& p2pJSON = json[ConfigProps::P2P::P2P];

			if (p2pJSON.isMember(ConfigProps::P2P::MAX_PEERS)) {
				m_maxConnections = p2pJSON.get(ConfigProps::P2P::MAX_PEERS, 60).asInt();
			}

			if (p2pJSON.isMember(ConfigProps::P2P::MIN_PEERS)) {
				m_minConnections = p2pJSON.get(ConfigProps::P2P::MIN_PEERS, 10).asInt();
			}

			if (p2pJSON.isMember(ConfigProps::P2P::PREFERRED_PEERS)) {
				Json::Value peers = p2pJSON.get(ConfigProps::P2P::PREFERRED_PEERS, Json::Value(Json::nullValue));
				for (auto& peer : peers) { 
					const std::string& addressStr = peer.asCString();
					const std::vector<IPAddress>& iPAddresses = GetValidIPAddress(addressStr);
					m_peferredPeers.insert(iPAddresses.begin(), iPAddresses.end());
				}
			}

			if (p2pJSON.isMember(ConfigProps::P2P::ALLOWED_PEERS)) {
				Json::Value peers = p2pJSON.get(ConfigProps::P2P::ALLOWED_PEERS, Json::Value(Json::nullValue));
				for (auto& peer : peers) {
					const std::string& addressStr = peer.asCString();
					const std::vector<IPAddress>& iPAddresses = GetValidIPAddress(addressStr);
					m_allowedPeers.insert(iPAddresses.begin(), iPAddresses.end());
				}
			}

			if (p2pJSON.isMember(ConfigProps::P2P::BLOCKED_PEERS)) {
				Json::Value peers = p2pJSON.get(ConfigProps::P2P::BLOCKED_PEERS, Json::Value(Json::nullValue));
				for (auto& peer : peers) {
					const std::string& addressStr = peer.asCString();
					const std::vector<IPAddress>& iPAddresses = GetValidIPAddress(addressStr);
					m_blockedPeers.insert(iPAddresses.begin(), iPAddresses.end());
				}
			}
		}

		if (env == Environment::AUTOMATED_TESTING) {
			m_minSyncPeers = 1;
		} else {
			m_minSyncPeers = 4;
		}
	}

private:
	int m_maxConnections;
	int m_minConnections;
	uint16_t m_port;
	std::vector<uint8_t> m_magicBytes;
	uint8_t m_minSyncPeers;
	std::unordered_set<IPAddress> m_peferredPeers;
	std::unordered_set<IPAddress> m_allowedPeers;
	std::unordered_set<IPAddress> m_blockedPeers;

	static std::vector<IPAddress> GetValidIPAddress(const std::string& addressStr)
    {
		std::vector<IPAddress> ipList;

        if (IPAddress::IsValidIPAddress(addressStr)) {
			try {
				ipList.push_back(IPAddress::Parse(addressStr));
			} catch (std::exception& e) {
				LOG_DEBUG_F("Error parsing IP Address: {} {}", addressStr, e.what());
			}
		} else {
			LOG_INFO_F("Resolving domain: {}", addressStr);
			try {
				for (const IPAddress ipAddress : IPAddress::Resolve(addressStr))
				{
					LOG_INFO_F("Resolved domain: {}", ipAddress);
					ipList.push_back(ipAddress);
				}
			} catch (std::exception& e) {
				LOG_DEBUG_F("Failed to resolve domain: {} {}", addressStr, e.what());
			}
		}

		return ipList;
    };
};