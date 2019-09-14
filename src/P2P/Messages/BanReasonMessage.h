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
	virtual BanReasonMessage* Clone() const override final { return new BanReasonMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::BanReasonMsg; }
	inline const uint32_t GetBanReason() const { return m_banReason; }

	//
	// Deserialization
	//
	static BanReasonMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const uint32_t banReason = byteBuffer.ReadU32();

		return BanReasonMessage(banReason);
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.Append<uint32_t>(m_banReason);
	}

private:
	// Ban Reason
	uint32_t m_banReason; // TODO: Create EBanReason enum
};