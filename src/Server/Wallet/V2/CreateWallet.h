#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Core/Util/JsonUtil.h>
#include <optional>

class CreateWallet : RPCMethod
{
public:
	CreateWallet(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~CreateWallet() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		const Json::Value& params = request.GetParams().value();
		std::string username = StringUtil::ToLower(JsonUtil::GetRequiredString(params, "username"));
		SecureString password(JsonUtil::GetRequiredString(params, "password"));
		const uint64_t numWords = JsonUtil::GetRequiredUInt64(params, "num_words");

		auto wallet = m_pWalletManager->InitializeNewWallet(username, password, (int)numWords);

		Json::Value result;
		result["status"] = "SUCCESS";
		result["session_token"] = wallet.second.ToBase64();
		result["wallet_seed"] = std::string(wallet.first);
		return request.BuildResult(result);
	}

private:
	std::optional<RPC::Error> ValidateInput(const std::string& username, const SecureString& password, const uint64_t numWords)
	{
		// TODO: Should we allow usernames to contain spaces or special characters?

		std::vector<std::string> accounts = m_pWalletManager->GetAllAccounts();
		for (const std::string& account : accounts)
		{
			if (StringUtil::ToLower(account) == username)
			{
				// TODO: Username already exists

			}
		}

		if (password.empty())
		{
			// TODO: Invalid password
		}

		if (numWords < 12 || numWords > 24 || numWords % 3 != 0)
		{
			// TODO: Invalid num_words
		}
	}

	IWalletManagerPtr m_pWalletManager;
};