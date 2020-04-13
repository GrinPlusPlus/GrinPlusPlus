#pragma once

#include <Common/Util/StringUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Infrastructure/Logger.h>
#include <API/Wallet/Owner/Models/LoginCriteria.h>
#include <optional>

class LoginHandler : RPCMethod
{
public:
	LoginHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~LoginHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		LoginCriteria criteria = LoginCriteria::FromJSON(request.GetParams().value());
		ValidateInput(criteria);

		auto response = m_pWalletManager->Login(criteria);

		return request.BuildResult(response.ToJSON());
	}

private:
	void ValidateInput(const LoginCriteria& criteria) const
	{
		std::vector<std::string> accounts = m_pWalletManager->GetAllAccounts();
		for (const std::string& account : accounts)
		{
			if (StringUtil::ToLower(account) == criteria.GetUsername())
			{
				return;
			}
		}

		WALLET_ERROR_F("Failed to login as user {}. Username not found.", criteria.GetUsername());
		throw API_EXCEPTION_F(
			RPC::ErrorCode::INVALID_PARAMS,
			"Username {} not found",
			criteria.GetUsername()
		);
	}

	IWalletManagerPtr m_pWalletManager;
};