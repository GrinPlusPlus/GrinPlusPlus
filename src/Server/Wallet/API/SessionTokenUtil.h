#pragma once

#include <civetweb.h>

#include <Net/Util/HTTPUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Exceptions/SessionTokenException.h>

class SessionTokenUtil
{
public:
	static SessionToken GetSessionToken(mg_connection& connection)
	{
		const std::optional<std::string> sessionTokenOpt = HTTPUtil::GetHeaderValue(&connection, "session_token");
		if (!sessionTokenOpt.has_value())
		{
			throw SessionTokenException();
		}

		try
		{
			return SessionToken::FromBase64(*sessionTokenOpt);
		}
		catch (const DeserializationException&)
		{
			throw SessionTokenException();
		}
	}
};