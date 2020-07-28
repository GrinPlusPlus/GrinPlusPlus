#pragma once

#include "Message.h"

class BanReasonMessage : public IMessage
{
public:
	//
	// Constructors
	//
	BanReasonMessage(const uint32_t banReason)
		: m_banReason(banReason)
	{

	}
	BanReasonMessage(const BanReasonMessage& other) = default;
	BanReasonMessage(BanReasonMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~BanReasonMessage() = default;

	//
	// Operators
	//
	BanReasonMessage& operator=(const BanReasonMessage& other) = default;
	BanReasonMessage& operator=(BanReasonMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new BanReasonMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::BanReasonMsg; }
	uint32_t GetBanReason() const { return m_banReason; }

	//
	// Deserialization
	//
	static BanReasonMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const uint32_t banReason = byteBuffer.ReadU32();

		return BanReasonMessage(banReason);
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.Append<uint32_t>(m_banReason);
	}

private:
	// Ban Reason
	uint32_t m_banReason; // TODO: Create EBanReason enum
};