#pragma once

#include <Common/Util/StringUtil.h>
#include <Core/Exceptions/GrinException.h>
#include <P2P/BanReason.h>

#define BAD_DATA_EXCEPTION(reason, msg) BadDataException(reason, msg, __func__)
#define BAD_DATA_EXCEPTION_F(reason, msg, ...) BadDataException(reason, StringUtil::Format(msg, __VA_ARGS__), __func__)

class BadDataException : public GrinException
{
public:
	BadDataException(EBanReason reason, const std::string& message, const std::string& function)
		: m_reason(reason), GrinException(message, function) { }

	EBanReason GetReason() const noexcept { return m_reason; }
	std::string GetReasonStr() const noexcept { return BanReason::Format(m_reason); }

private:
	EBanReason m_reason;
};