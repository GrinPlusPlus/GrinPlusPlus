#pragma once

#include "MessageHeader.h"

#include <vector>

class RawMessage
{
public:
	//
	// Constructors
	//
	RawMessage(MessageHeader&& messageHeader, std::vector<unsigned char>&& payload)
		: m_messageHeader(messageHeader), m_payload(std::move(payload))
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
	const MessageHeader& GetMessageHeader() const { return m_messageHeader; }
	const std::vector<unsigned char>& GetPayload() const { return m_payload; }

private:
	MessageHeader m_messageHeader;
	std::vector<unsigned char> m_payload;
};