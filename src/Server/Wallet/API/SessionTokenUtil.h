#pragma once

#include "../../civetweb/include/civetweb.h"
#include "../../RestUtil.h"

#include <Wallet/SessionToken.h>
#include <Wallet/Exceptions/SessionTokenException.h>

class SessionTokenUtil
{
public:
	static SessionToken GetSessionToken(mg_connection& connection)
	{
		const std::optional<std::string> sessionTokenOpt = RestUtil::GetHeaderValue(&connection, "session_token");
		if (!sessionTokenOpt.has_value())
		{
			throw SessionTokenException();
		}

		try
		{
			return SessionToken::FromBase64(sessionTokenOpt.value());
		}
		catch (const DeserializationException&)
		{
			throw SessionTokenException();
		}
	}

};