#pragma once

#include "Message.h"
#include <Crypto/Models/Hash.h>
#include <Net/SocketAddress.h>
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
		Hash hash,
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
	IMessagePtr Clone() const final { return IMessagePtr(new HandMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::Hand; }
	uint32_t GetVersion() const { return m_version; }
	const Capabilities& GetCapabilities() const { return m_capabilities; }
	uint64_t GetNonce() const { return m_nonce; }
	const Hash& GetHash() const { return m_hash; }
	uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	const SocketAddress& GetSenderAddress() const { return m_senderAddress; }
	const SocketAddress& GetReceiverAddress() const { return m_receiverAddress; }
	const std::string& GetUserAgent() const { return m_userAgent; }

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
		Hash hash = byteBuffer.ReadBigInteger<32>();

		return HandMessage(version, capabilities, nonce, std::move(hash), totalDifficulty, std::move(senderAddress), std::move(receiverAddress), userAgent);
	}

protected:
	void SerializeBody(Serializer& serializer) const final
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
	Hash m_hash;

	// Total difficulty accumulated by the sender, used to check whether sync may be needed.
	uint64_t m_totalDifficulty;

	// Network address of the sender
	SocketAddress m_senderAddress;

	// Network address of the receiver
	SocketAddress m_receiverAddress;

	// Name and version of the software
	std::string m_userAgent;
};