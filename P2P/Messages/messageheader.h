#pragma once

#include "MessageTypes.h"

#include <vector>
#include <stdint.h>
#include <Config/Config.h>
#include <Core/Serialization/ByteBuffer.h>

class MessageHeader
{
public:
	//
	// Constructors
	//
	MessageHeader(const std::vector<unsigned char>& magicBytes, const MessageTypes::EMessageType messageType, const uint64_t messageLength)
		: m_magicBytes(magicBytes), m_messageType(messageType), m_messageLength(messageLength)
	{

	}
	MessageHeader(const MessageHeader& other) = default;
	MessageHeader(MessageHeader&& other) noexcept = default;
	MessageHeader() = default;

	//
	// Destructor
	//
	~MessageHeader() = default;

	//
	// Operators
	//
	MessageHeader& operator=(const MessageHeader& other) = default;
	MessageHeader& operator=(MessageHeader&& other) noexcept = default;

	//
	// Getters
	//
	inline const std::vector<unsigned char>& GetMagicBytes() const { return m_magicBytes; }
	inline const MessageTypes::EMessageType GetMessageType() const { return m_messageType; }
	inline const uint64_t GetMessageLength() const { return m_messageLength; }

	//
	// Validation
	//
	inline const bool IsValid(const Config& config) const
	{
		if (m_magicBytes[0] == config.GetEnvironment().GetMagicBytes()[0] && m_magicBytes[1] == config.GetEnvironment().GetMagicBytes()[1])
		{
			if (m_messageLength <= (MessageTypes::GetMaximumSize(m_messageType) * 4))
			{
				return true;
			}
		}

		return false;
	}

	//
	// Deserialization
	//
	static MessageHeader Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t magicByte1 = byteBuffer.ReadU8();
		const uint8_t magicByte2 = byteBuffer.ReadU8();

		const uint8_t messageType = byteBuffer.ReadU8();
		const uint64_t messageLength = byteBuffer.ReadU64();

		const std::vector<unsigned char> magicBytes = { magicByte1, magicByte2 };
		return MessageHeader(magicBytes, (MessageTypes::EMessageType) messageType, messageLength);
	}

private:
	std::vector<unsigned char> m_magicBytes;
	MessageTypes::EMessageType m_messageType;
	uint64_t m_messageLength;
};