#pragma once

#include "Message.h"

#include <Crypto/BigInteger.h>

class GetHeadersMessage : public IMessage
{
public:
	//
	// Constructors
	//
	GetHeadersMessage(std::vector<CBigInteger<32>>&& hashes)
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
	virtual GetHeadersMessage* Clone() const override final { return new GetHeadersMessage(*this); }

	//
	// Getters
	//
	virtual MessageTypes::EMessageType GetMessageType() const override final { return MessageTypes::GetHeaders; }
	inline const std::vector<CBigInteger<32>>& GetHashes() const { return m_hashes; }

	//
	// Deserialization
	//
	static GetHeadersMessage Deserialize(ByteBuffer& byteBuffer)
	{
		std::vector<CBigInteger<32>> hashes;

		const uint8_t numHashes = byteBuffer.ReadU8();
		for (uint8_t i = 0; i < numHashes; i++)
		{
			hashes.emplace_back(byteBuffer.ReadBigInteger<32>());
		}

		return GetHeadersMessage(std::move(hashes));
	}

protected:
	virtual void SerializeBody(Serializer& serializer) const override final
	{
		serializer.Append<uint8_t>((uint8_t)m_hashes.size());
		for (auto iter = m_hashes.cbegin(); iter != m_hashes.cend(); iter++)
		{
			serializer.AppendBigInteger<32>(*iter);
		}
	}

private:
	std::vector<CBigInteger<32>> m_hashes;
};