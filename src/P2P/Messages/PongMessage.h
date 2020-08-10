#pragma once

#include "Message.h"

class PongMessage : public IMessage
{
public:
	//
	// Constructors
	//
	PongMessage(const uint64_t totalDifficulty, const uint64_t height)
		: m_totalDifficulty(totalDifficulty), m_height(height)
	{

	}
	PongMessage(const PongMessage& other) = default;
	PongMessage(PongMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~PongMessage() = default;

	//
	// Operators
	//
	PongMessage& operator=(const PongMessage& other) = default;
	PongMessage& operator=(PongMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new PongMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::Pong; }
	uint64_t GetTotalDifficulty() const { return m_totalDifficulty; }
	uint64_t GetHeight() const { return m_height; }

	//
	// Deserialization
	//
	static PongMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t totalDifficulty = byteBuffer.ReadU64();
		const uint64_t height = byteBuffer.ReadU64();

		return PongMessage(totalDifficulty, height);
	}

protected:
	void SerializeBody(Serializer& serializer) const final
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