#pragma once

#include "Message.h"

class ErrorMessage : public IMessage
{
public:
	//
	// Constructors
	//
	ErrorMessage(const uint32_t errorCode, const std::string& message)
		: m_errorCode(errorCode), m_message(message)
	{

	}
	ErrorMessage(const ErrorMessage& other) = default;
	ErrorMessage(ErrorMessage&& other) noexcept = default;

	//
	// Destructor
	//
	virtual ~ErrorMessage() = default;

	//
	// Operators
	//
	ErrorMessage& operator=(const ErrorMessage& other) = default;
	ErrorMessage& operator=(ErrorMessage&& other) noexcept = default;

	//
	// Clone
	//
	IMessagePtr Clone() const final { return IMessagePtr(new ErrorMessage(*this)); }

	//
	// Getters
	//
	MessageTypes::EMessageType GetMessageType() const final { return MessageTypes::Error; }
	uint32_t GetErrorCode() const { return m_errorCode; }
	const std::string& GetErrorMessage() const { return m_message; }

	//
	// Deserialization
	//
	static ErrorMessage Deserialize(ByteBuffer& byteBuffer)
	{
		const uint32_t errorCode = byteBuffer.ReadU32();
		const std::string message = byteBuffer.ReadVarStr();

		return ErrorMessage(errorCode, message);
	}

protected:
	void SerializeBody(Serializer& serializer) const final
	{
		serializer.Append<uint32_t>(m_errorCode);
		serializer.AppendVarStr(m_message);
	}

private:
	// Error Code
	const uint32_t m_errorCode; // TODO: Create EErrorType enum

	// Error Message
	const std::string& m_message;
};