#pragma once

#include "Message.h"

#include <Crypto/Models/Hash.h>

class GetHeadersMessage : public IMessage
{
public:
	//
	// Constructors
	//
	GetHeadersMessage(std::vector<Hash>&& hashes)
		: m_hashes(std::move(hashes))
	{

	}
	GetHeadersMessage(const GetHeadersMessage& other) = default;
	GetHeadersMessage(GetHeadersMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~GetHeadersMessage() = default;

	//
	// Operators
	//
	GetHeadersMessage& operator=(const GetHeadersMessage& other) = default;
	GetHeadersMessage& operator=(GetHeadersMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new GetHeadersMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::GetHeaders; }
	const std::vector<Hash>& GetHashes() const { return m_hashes; }

	//
	// Deserialization
	//
	static GetHeadersMessage Deserialize(ByteBuffer& byteBuffer)
	{
		std::vector<Hash> hashes;

		const uint8_t numHashes = byteBuffer.ReadU8();
		for (uint8_t i = 0; i < numHashes; i++)
		{
			hashes.emplace_back(byteBuffer.ReadBigInteger<32>());
		}

		return GetHeadersMessage(std::move(hashes));
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.Append<uint8_t>((uint8_t)m_hashes.size());
		for (auto iter = m_hashes.cbegin(); iter != m_hashes.cend(); iter++)
		{
			serializer.AppendBigInteger<32>(*iter);
		}
	}

private:
	std::vector<Hash> m_hashes;
};