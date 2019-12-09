#pragma once

#include <Net/Clients/RPC/RPC.h>

namespace RPC
{
	class Errors
	{
	private:
		static Json::Value GetJSON(const std::string& type)
		{
			Json::Value errorData;
			errorData["type"] = type;
			return errorData;
		}

	public:
		static inline RPC::Error RECEIVER_UNREACHABLE = RPC::Error(-101, "Failed to establish TOR connection", GetJSON("RECEIVER_UNREACHABLE"));
		static inline RPC::Error SLATE_VERSION_MISMATCH = RPC::Error(-102, "Unsupported slate version", GetJSON("SLATE_VERSION_MISMATCH"));
	};
}