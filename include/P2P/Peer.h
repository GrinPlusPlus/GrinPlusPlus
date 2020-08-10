#pragma once

#include <Net/IPAddress.h>
#include <Net/SocketAddress.h>
#include <P2P/Common.h>
#include <P2P/Capabilities.h>
#include <P2P/BanReason.h>
#include <Core/Traits/Printable.h>
#include <json/json.h>

#include <string>
#include <cstdint>
#include <ctime>
#include <atomic>
#include <chrono>
#include <memory>

class Peer : public Traits::IPrintable
{
public:
	//
	// Constructors
	//
	Peer(const IPAddress& ipAddress)
		: m_dirty(false),
		m_connected(false),
		m_ipAddress(ipAddress),
		m_version(0),
		m_capabilities(Capabilities(Capabilities::UNKNOWN)),
		m_userAgent(""),
		m_lastContactTime(0),
		m_lastBanTime(0),
		m_banReason(EBanReason::None),
		m_lastTxHashSetRequest(0)
	{

	}
	Peer(const IPAddress& ipAddress, const uint32_t version, const Capabilities& capabilities, const std::string& userAgent)
		: m_dirty(false),
		m_connected(false),
		m_ipAddress(ipAddress),
		m_version(version),
		m_capabilities(capabilities),
		m_userAgent(userAgent),
		m_lastContactTime(0),
		m_lastBanTime(0),
		m_banReason(EBanReason::None),
		m_lastTxHashSetRequest(0)
	{

	}
	Peer(const IPAddress& ipAddress, const uint32_t version, const Capabilities& capabilities, const std::string& userAgent, const std::time_t lastContactTime, const std::time_t lastBanTime, const EBanReason banReason)
		: m_dirty(false),
		m_connected(false),
		m_ipAddress(ipAddress),
		m_version(version), 
		m_capabilities(capabilities), 
		m_userAgent(userAgent), 
		m_lastContactTime(lastContactTime), 
		m_lastBanTime(lastBanTime),
		m_banReason(banReason),
		m_lastTxHashSetRequest(0)
	{

	}
	Peer(const Peer& other) = delete;
	Peer(Peer&& other) = delete;
	Peer() = default;

	//
	// Destructor
	//
	virtual ~Peer() = default;

	//
	// Operators
	//
	Peer& operator=(const Peer& other)
	{
		m_ipAddress = other.m_ipAddress;
		m_version = other.m_version;
		m_capabilities = other.m_capabilities.load();
		m_userAgent = other.m_userAgent;
		m_lastContactTime = other.m_lastContactTime.load();
		m_lastBanTime = other.m_lastBanTime.load();
		m_banReason = other.m_banReason.load();
		m_lastTxHashSetRequest = other.m_lastTxHashSetRequest.load();
		return *this;
	}
	Peer& operator=(Peer&& other) = default;

	//
	// Setters
	//
	void SetDirty(const bool value) noexcept { m_dirty = value; }
	void SetConnected(const bool connected) noexcept
	{
		m_connected = connected;
		m_dirty = true;
	}
	void UpdateVersion(const uint32_t version) noexcept
	{
		m_version = version;
		m_dirty = true;
	}
	void UpdateCapabilities(const Capabilities& capabilities) noexcept { m_capabilities = capabilities; }
	void UpdateLastContactTime() const noexcept { m_lastContactTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); }
	void Ban(const EBanReason banReason) noexcept
	{
		m_lastBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		m_banReason = banReason;
		m_dirty = true;
	}
	void Unban() noexcept
	{
		m_lastBanTime = 0;
		m_dirty = true;
	}
	void UpdateUserAgent(const std::string& userAgent) noexcept { m_userAgent = userAgent; }
	void UpdateLastTxHashSetRequest() noexcept { m_lastTxHashSetRequest = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); }

	//
	// Getters
	//
	bool IsDirty() const noexcept { return m_dirty; }
	bool IsConnected() const noexcept { return m_connected; }
	const IPAddress& GetIPAddress() const noexcept { return m_ipAddress; }
	uint32_t GetVersion() const noexcept { return m_version; }
	Capabilities GetCapabilities() const noexcept { return m_capabilities; }
	const std::string& GetUserAgent() const noexcept { return m_userAgent; }
	std::time_t GetLastContactTime() const noexcept { return m_lastContactTime; }
	std::time_t GetLastBanTime() const noexcept { return m_lastBanTime; }
	EBanReason GetBanReason() const noexcept { return m_banReason; }
	std::time_t GetLastTxHashSetRequest() const noexcept { return m_lastTxHashSetRequest; }
	bool IsBanned() const noexcept
	{
		const time_t maxBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW));
		return GetLastBanTime() > maxBanTime;
	}

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		SocketAddress(m_ipAddress, 0).Serialize(serializer);
		serializer.Append<uint32_t>(m_version);
		m_capabilities.load().Serialize(serializer);
		serializer.AppendVarStr(m_userAgent);
		serializer.Append<int64_t>(m_lastContactTime.load());
		serializer.Append<int64_t>(m_lastBanTime.load());
		serializer.Append<int32_t>(m_banReason.load());
	}

	static std::shared_ptr<Peer> Deserialize(ByteBuffer& byteBuffer)
	{
		SocketAddress socketAddress = SocketAddress::Deserialize(byteBuffer);
		uint32_t version = byteBuffer.ReadU32();
		Capabilities capabilities = Capabilities::Deserialize(byteBuffer);
		std::string userAgent = byteBuffer.ReadVarStr();
		std::time_t lastContactTime = (std::time_t)byteBuffer.Read64();
		std::time_t lastBanTime = (std::time_t)byteBuffer.Read64();
		EBanReason banReason = (EBanReason)byteBuffer.Read32();

		return std::shared_ptr<Peer>(new Peer(
			socketAddress.GetIPAddress(),
			version,
			capabilities,
			userAgent,
			lastContactTime,
			lastBanTime,
			banReason
		));
	}

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["addr"] = GetIPAddress().Format();
		json["capabilities"] = GetCapabilities().ToJSON();
		json["user_agent"] = GetUserAgent();
		json["flags"] = IsBanned() ? "Banned" : "Healthy";
		json["last_banned"] = Json::UInt64(GetLastBanTime());
		json["ban_reason"] = BanReason::Format(GetBanReason());
		json["last_connected"] = Json::UInt64(GetLastContactTime());
		json["protocol_version"] = Json::UInt(GetVersion());
		return json;
	}

	std::string Format() const final { return m_ipAddress.Format(); }

private:
	std::atomic_bool m_dirty;
	std::atomic_bool m_connected;
	IPAddress m_ipAddress;
	uint32_t m_version;
	std::atomic<Capabilities> m_capabilities;
	std::string m_userAgent;
	mutable std::atomic<std::time_t> m_lastContactTime;
	std::atomic<std::time_t> m_lastBanTime;
	std::atomic<EBanReason> m_banReason;
	std::atomic<std::time_t> m_lastTxHashSetRequest;
};

typedef std::shared_ptr<Peer> PeerPtr;
typedef std::shared_ptr<const Peer> PeerConstPtr;