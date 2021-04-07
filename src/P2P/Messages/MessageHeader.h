#pragma once

#include "MessageTypes.h"

#include <vector>
#include <cstdint>
#include <Core/Global.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Printable.h>
#include <Core/Traits/Serializable.h>

class MessageHeader : public Traits::IPrintable, public Traits::ISerializable
{
public:
	//
	// Constructors
	//
	MessageHeader(std::vector<uint8_t> magicBytes, const MessageTypes::EMessageType messageType, const uint64_t messageLength)
		: m_magicBytes(std::move(magicBytes)), m_type(messageType), m_length(messageLength)
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
	const std::vector<uint8_t>& GetMagicBytes() const { return m_magicBytes; }
	MessageTypes::EMessageType GetMessageType() const { return m_type; }
	MessageTypes::EMessageType GetType() const { return m_type; }
	uint64_t GetMessageLength() const { return m_length; }
	uint64_t GetLength() const { return m_length; }

	//
	// Deserialization
	//
	void Serialize(Serializer& serializer) const noexcept
	{
		serializer
			.Append<uint8_t>(Global::GetMagicBytes()[0])
			.Append<uint8_t>(Global::GetMagicBytes()[1])
			.Append<uint8_t>((uint8_t)m_type)
			.Append<uint64_t>(m_length);
	}

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

		const std::vector<uint8_t> magicBytes = { magicByte1, magicByte2 };
		return MessageHeader(magicBytes, (MessageTypes::EMessageType) messageType, messageLength);
	}

	std::string Format() const noexcept final
	{
		return StringUtil::Format("{}:{}b", MessageTypes::ToString(m_type), m_length);
	}

private:
	std::vector<uint8_t> m_magicBytes;
	MessageTypes::EMessageType m_type;
	uint64_t m_length;
};