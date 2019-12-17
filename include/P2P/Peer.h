#pragma once

#include <Net/IPAddress.h>
#include <Net/SocketAddress.h>
#include <P2P/Common.h>
#include <P2P/Capabilities.h>
#include <P2P/BanReason.h>
#include <Core/Traits/Printable.h>

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
	Peer(const SocketAddress& socketAddress)
		: m_dirty(false),
		m_connected(false),
		m_socketAddress(socketAddress),
		m_version(0),
		m_capabilities(Capabilities(Capabilities::UNKNOWN)),
		m_userAgent(""),
		m_lastContactTime(0),
		m_lastBanTime(0),
		m_banReason(EBanReason::None),
		m_lastTxHashSetRequest(0)
	{

	}
	Peer(const SocketAddress& socketAddress, const uint32_t version, const Capabilities& capabilities, const std::string& userAgent)
		: m_dirty(false),
		m_connected(false),
		m_socketAddress(socketAddress),
		m_version(version),
		m_capabilities(capabilities),
		m_userAgent(userAgent),
		m_lastContactTime(0),
		m_lastBanTime(0),
		m_banReason(EBanReason::None),
		m_lastTxHashSetRequest(0)
	{

	}
	Peer(SocketAddress&& socketAddress, const uint32_t version, const Capabilities& capabilities, const std::string& userAgent, const std::time_t lastContactTime, const std::time_t lastBanTime, const EBanReason banReason)
		: m_dirty(false),
		m_connected(false),
		m_socketAddress(std::move(socketAddress)),
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
		m_socketAddress = other.m_socketAddress;
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
	void SetDirty(const bool value) { m_dirty = value; }
	void SetConnected(const bool connected)
	{
		m_connected = connected;
		m_dirty = true;
	}
	void UpdateVersion(const uint32_t version) { m_version = version; }
	void UpdateCapabilities(const Capabilities& capabilities) { m_capabilities = capabilities; }
	void UpdateLastContactTime() const { m_lastContactTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); }
	void Ban(const EBanReason banReason)
	{
		m_lastBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		m_banReason = banReason;
		m_dirty = true;
	}
	void Unban()
	{
		m_lastBanTime = 0;
		m_dirty = true;
	}
	void UpdateUserAgent(const std::string& userAgent) { m_userAgent = userAgent; }
	void UpdateLastTxHashSetRequest() { m_lastTxHashSetRequest = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()); }

	//
	// Getters
	//
	bool IsDirty() const { return m_dirty; }
	bool IsConnected() const { return m_connected; }
	const SocketAddress& GetSocketAddress() const { return m_socketAddress; }
	const IPAddress& GetIPAddress() const { return m_socketAddress.GetIPAddress(); }
	uint16_t GetPortNumber() const { return m_socketAddress.GetPortNumber(); }
	uint32_t GetVersion() const { return m_version; }
	Capabilities GetCapabilities() const { return m_capabilities; }
	const std::string& GetUserAgent() const { return m_userAgent; }
	std::time_t GetLastContactTime() const { return m_lastContactTime; }
	std::time_t GetLastBanTime() const { return m_lastBanTime; }
	EBanReason GetBanReason() const { return m_banReason; }
	std::time_t GetLastTxHashSetRequest() const { return m_lastTxHashSetRequest; }
	bool IsBanned() const
	{
		const time_t maxBanTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now() - std::chrono::seconds(P2P::BAN_WINDOW));
		return GetLastBanTime() > maxBanTime;
	}

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		m_socketAddress.Serialize(serializer);
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
			std::move(socketAddress),
			version,
			capabilities,
			userAgent,
			lastContactTime,
			lastBanTime,
			banReason
		));
	}

	virtual std::string Format() const override final { return m_socketAddress.GetIPAddress().Format(); }

private:
	std::atomic_bool m_dirty;
	std::atomic_bool m_connected;
	SocketAddress m_socketAddress;
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