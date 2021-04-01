#pragma once

#include "MessageHeader.h"

#include <vector>

class RawMessage : public Traits::IPrintable
{
public:
	//
	// Constructors
	//
	RawMessage(MessageHeader&& messageHeader, std::vector<unsigned char>&& payload)
		: m_header(messageHeader), m_payload(std::move(payload))
	{

	}
	RawMessage(const RawMessage& other) = default;
	RawMessage(RawMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~RawMessage() = default;

	//
	// Operators
	//
	RawMessage& operator=(const RawMessage& other) = default;
	RawMessage& operator=(RawMessage&& other) noexcept = default;

	//
	// Getters
	//
	const MessageHeader& GetMessageHeader() const { return m_header; }
	MessageTypes::EMessageType GetMessageType() const { return m_header.GetMessageType(); }
	const std::vector<unsigned char>& GetPayload() const { return m_payload; }

	std::string Format() const noexcept final
	{
		return StringUtil::Format("RawMessage({})", m_header.Format());
	}

private:
	MessageHeader m_header;
	std::vector<unsigned char> m_payload;
};