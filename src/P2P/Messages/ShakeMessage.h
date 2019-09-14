#pragma once

#include "Message.h"
#include <P2P/SocketAddress.h>
#include <P2P/Capabilities.h>

// Second part of a handshake, receiver of the first part replies with its own version and characteristics.
class ShakeMessage : public IMessage
{
public:
	//
	// Constructors
	//
	ShakeMessage(
		const uint32_t version, 
		const Capabilities& capabilities,
		CBigInteger<32>&& hash,
		const uint64_t totalDifficulty,
		const std::string& userAgent
	) 
		: m_version(version), m_capabilities(capabilities), m_hash(std::move(hash)), m_totalDifficulty(totalDifficulty), m_userAgent(userAgent)
	{

	}
	ShakeMessage(const ShakeMessage& other) = default;
	ShakeMessage(ShakeMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~ShakeMessage() = default;

	//
	// Operators
	//
	ShakeMessage& operator=(const ShakeMessage& other) = default;
	ShakeMessage& operator=(ShakeMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual ShakeMessage* Clone() const override final { return new ShakeMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::Shake; }
	inline const uint32_t GetVersion() const { return m_version; }
	inline const Capabilities& GetCapabilities() const { return m_capabilities; }
	inline const CBigInteger<32>& GetHash() const { return m_hash; }
	inline const uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	inline const std::string& GetUserAgent() const { return m_userAgent; }

	//
	// Deserialization
	//
	static ShakeMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const uint32_t version = byteBuffer.ReadU32();
		const Capabilities capabilities = Capabilities::Deserialize(byteBuffer);
		const uint64_t totalDifficulty = byteBuffer.ReadU64();
		const std::string userAgent = byteBuffer.ReadVarStr();
		CBigInteger<32> hash = byteBuffer.ReadBigInteger<32>();

		return ShakeMessage(version, capabilities, std::move(hash), totalDifficulty, userAgent);
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.Append<uint32_t>(m_version);
		m_capabilities.Serialize(serializer);
		serializer.Append<uint64_t>(m_totalDifficulty);
		serializer.AppendVarStr(m_userAgent);
		serializer.AppendBigInteger(m_hash);
	}

private:
	// Protocol version of the sender
	uint32_t m_version;

	// Capabilities of the sender
	Capabilities m_capabilities;

	// Genesis block of our chain, only connect to peers on the same chain
	CBigInteger<32> m_hash;

	// Total difficulty accumulated by the sender, used to check whether sync may be needed.
	uint64_t m_totalDifficulty;

	// Name of version of the software
	std::string m_userAgent;
};