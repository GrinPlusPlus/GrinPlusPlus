#pragma once

#include "Message.h"

class PingMessage : public IMessage
{
public:
	//
	// Constructors
	//
	PingMessage(const uint64_t totalDifficulty, const uint64_t height)
		: m_totalDifficulty(totalDifficulty), m_height(height)
	{

	}
	PingMessage(const PingMessage& other) = default;
	PingMessage(PingMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~PingMessage() = default;

	//
	// Operators
	//
	PingMessage& operator=(const PingMessage& other) = default;
	PingMessage& operator=(PingMessage&& other) noexcept = default;

	//
	// Clone
	//
	virtual PingMessage* Clone() const override final { return new PingMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::Ping; }
	inline const uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	inline const uint64_t GetHeight() const { return m_height; }

	//
	// Deserialization
	//
	static PingMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t totalDifficulty = byteBuffer.ReadU64();
		const uint64_t height = byteBuffer.ReadU64();

		return PingMessage(totalDifficulty, height);
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.Append<uint64_t>(m_totalDifficulty);
		serializer.Append<uint64_t>(m_height);
	}

private:
	// Total difficulty accumulated by the sender, used to check whether sync may be needed.
	const uint64_t m_totalDifficulty;

	// Total height
	const uint64_t m_height;
};