#pragma once

#include <exception>
#include <system_error>

#include <Common/Logger.h>

class SocketException : public std::exception
{
public:
	SocketException(const std::error_code& ec, const std::string& message)
	{
		m_message = "SocketException:";

		if (ec) {
			m_message += StringUtil::Format(" [{}: {}]", ec.value(), ec.message());
		}

		if (!message.empty()) {
			m_message += " " + message;
		}
	}

	const char* what() const throw()
	{
		return m_message.c_str();
	}

private:
	std::string m_message;
};