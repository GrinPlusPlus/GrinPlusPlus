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

		static inline RPC::Error PARAMS_MISSING = RPC::Error(-150, "'params' missing", GetJSON("PARAMS_MISSING"));

		// create_wallet (-200 -> -209)
		static inline RPC::Error USER_ALREADY_EXISTS = RPC::Error(-200, "User already exists", GetJSON("USER_ALREADY_EXISTS"));
		static inline RPC::Error PASSWORD_CRITERIA_NOT_MET = RPC::Error(-201, "Password doesn't mean requirements", GetJSON("PASSWORD_CRITERIA_NOT_MET"));
		static inline RPC::Error INVALID_NUM_SEED_WORDS = RPC::Error(-202, "Invalid number of seed words", GetJSON("INVALID_NUM_SEED_WORDS"));

		// login (-210 -> -219)
		static inline RPC::Error USER_DOESNT_EXIST = RPC::Error(-210, "User doesn't exist", GetJSON("USER_DOESNT_EXIST"));
		static inline RPC::Error INVALID_PASSWORD = RPC::Error(-211, "Invalid Password", GetJSON("INVALID_PASSWORD"));

		// list_txs (-240 -> -249)
		static inline RPC::Error INVALID_STATUSES = RPC::Error(-240, "Invalid statuses", GetJSON("INVALID_STATUSES"));

		// cancel_tx (-250 -> -259)
		static inline RPC::Error TX_ID_MISSING = RPC::Error(-250, "'tx_id' missing", GetJSON("TX_ID_MISSING"));
	};
}