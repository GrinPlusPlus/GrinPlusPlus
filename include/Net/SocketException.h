#pragma once

#include <exception>
#include <system_error>

#include <Infrastructure/Logger.h>

class SocketException : public std::exception
{
public:
	SocketException(const std::error_code& ec)
	{
		if (ec)
		{
			LOG_DEBUG_F("Socket exception occurred: ({}) {}", ec.value(), ec.message());
		}
	}

	const char* what() const throw()
	{
		return "Socket exception occurred.";
	}
};