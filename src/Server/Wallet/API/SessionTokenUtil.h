#pragma once

#include <Net/Util/HTTPUtil.h>
#include <Wallet/SessionToken.h>
#include <Wallet/Exceptions/SessionTokenException.h>

// Forward Declarations
struct mg_connection;

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
			return SessionToken::FromBase64(sessionTokenOpt.value());
		}
		catch (const DeserializationException&)
		{
			throw SessionTokenException();
		}
	}
};