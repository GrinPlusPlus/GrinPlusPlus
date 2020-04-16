#pragma once

#include <Core/Exceptions/GrinException.h>
#include <Common/Util/StringUtil.h>
#include <Net/Clients/RPC/RPC.h>

#define API_EXCEPTION(errorCode, msg) APIException(errorCode, msg, __func__)
#define API_EXCEPTION_F(errorCode, msg, ...) APIException(errorCode, StringUtil::Format(msg, __VA_ARGS__), __func__)

class APIException : public GrinException
{
public:
	APIException(const int ec, const std::string& message, const std::string& function)
		: GrinException(message, function), m_errorCode(ec)
	{

	}

	int GetErrorCode() const noexcept { return m_errorCode; }

private:
	int m_errorCode;
};