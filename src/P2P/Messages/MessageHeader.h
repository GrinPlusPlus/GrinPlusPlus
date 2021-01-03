#pragma once

#include "MessageTypes.h"

#include <vector>
#include <cstdint>
#include <Core/Global.h>
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
	const std::vector<unsigned char>& GetMagicBytes() const { return m_magicBytes; }
	MessageTypes::EMessageType GetMessageType() const { return m_messageType; }
	uint64_t GetMessageLength() const { return m_messageLength; }

	//
	// Deserialization
	//
	static MessageHeader Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t magicByte1 = byteBuffer.ReadU8();
		const uint8_t magicByte2 = byteBuffer.ReadU8();
		const std::vector<uint8_t>& magic_bytes = Global::GetMagicBytes();
		if (magicByte1 != magic_bytes[0] || magicByte2 != magic_bytes[1]) {
			throw DESERIALIZATION_EXCEPTION("Message header is invalid. Bad magic bytes.");
		}

		const uint8_t messageType = byteBuffer.ReadU8();
		const uint64_t messageLength = byteBuffer.ReadU64();

		if (messageLength > MessageTypes::GetMaximumSize((MessageTypes::EMessageType)messageType) * 4) {
			throw DESERIALIZATION_EXCEPTION("Message header is invalid. Message length too long");
		}

		const std::vector<unsigned char> magicBytes = { magicByte1, magicByte2 };
		return MessageHeader(magicBytes, (MessageTypes::EMessageType) messageType, messageLength);
	}

private:
	std::vector<unsigned char> m_magicBytes;
	MessageTypes::EMessageType m_messageType;
	uint64_t m_messageLength;
};