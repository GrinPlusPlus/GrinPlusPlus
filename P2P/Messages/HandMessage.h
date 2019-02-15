#pragma once

#include "Message.h"
#include <P2P/SocketAddress.h>
#include <P2P/Capabilities.h>

// First part of a handshake, sender advertises its version and characteristics.
class HandMessage : public IMessage
{
public:
	//
	// Constructors
	//
	HandMessage(
		const uint32_t version, 
		const Capabilities& capabilities, 
		const uint64_t nonce, 
		CBigInteger<32>&& hash,
		const uint64_t totalDifficulty, 
		SocketAddress&& senderAddress, 
		SocketAddress&& receiverAddress, 
		const std::string& userAgent
	) 
		: m_version(version), m_capabilities(capabilities), m_nonce(nonce), m_hash(std::move(hash)), m_totalDifficulty(totalDifficulty), 
		m_senderAddress(std::move(senderAddress)), m_receiverAddress(std::move(receiverAddress)), m_userAgent(userAgent)
	{

	}
	HandMessage(const HandMessage& other) = default;
	HandMessage(HandMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~HandMessage() = default;

	//
	// Operators
	//
	HandMessage& operator=(const HandMessage& other) = default;
	HandMessage& operator=(HandMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual HandMessage* Clone() const override final { return new HandMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::Hand; }
	inline const uint32_t GetVersion() const { return m_version; }
	inline const Capabilities& GetCapabilities() const { return m_capabilities; }
	inline const uint64_t GetNonce() const { return m_nonce; }
	inline const CBigInteger<32>& GetHash() const { return m_hash; }
	inline const uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	inline const SocketAddress& GetSenderAddress() const { return m_senderAddress; }
	inline const SocketAddress& GetReceiverAddress() const { return m_receiverAddress; }
	inline const std::string& GetUserAgent() const { return m_userAgent; }

	//
	// Deserialization
	//
	static HandMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const uint32_t version = byteBuffer.ReadU32();
		const Capabilities capabilities = Capabilities::Deserialize(byteBuffer);
		const uint64_t nonce = byteBuffer.ReadU64();
		const uint64_t totalDifficulty = byteBuffer.ReadU64();
		SocketAddress senderAddress = SocketAddress::Deserialize(byteBuffer);
		SocketAddress receiverAddress = SocketAddress::Deserialize(byteBuffer);
		const std::string userAgent = byteBuffer.ReadVarStr();
		CBigInteger<32> hash = byteBuffer.ReadBigInteger<32>();

		return HandMessage(version, capabilities, nonce, std::move(hash), totalDifficulty, std::move(senderAddress), std::move(receiverAddress), userAgent);
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.Append<uint32_t>(m_version);
		m_capabilities.Serialize(serializer);
		serializer.Append<uint64_t>(m_nonce);
		serializer.Append<uint64_t>(m_totalDifficulty);
		m_senderAddress.Serialize(serializer);
		m_receiverAddress.Serialize(serializer);
		serializer.AppendVarStr(m_userAgent);
		serializer.AppendBigInteger(m_hash);
	}

private:
	// Protocol version of the sender
	uint32_t m_version;

	// Capabilities of the sender
	Capabilities m_capabilities;

	// Randomly generated for each handshake, helps detect self
	uint64_t m_nonce;

	// Genesis block of our chain, only connect to peers on the same chain
	CBigInteger<32> m_hash;

	// Total difficulty accumulated by the sender, used to check whether sync may be needed.
	uint64_t m_totalDifficulty;

	// Network address of the sender
	SocketAddress m_senderAddress;

	// Network address of the receiver
	SocketAddress m_receiverAddress;

	// Name and version of the software
	std::string m_userAgent;
};