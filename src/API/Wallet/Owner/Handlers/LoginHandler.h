#pragma once

#include <Common/Util/StringUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Common/Logger.h>
#include <API/Wallet/Owner/Models/LoginCriteria.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Wallet/Exceptions/KeyChainException.h>
#include <optional>

class LoginHandler : public RPCMethod
{
public:
	LoginHandler(const IWalletManagerPtr& pWalletManager, const TorProcess::Ptr& pTorProcess)
		: m_pWalletManager(pWalletManager), m_pTorProcess(pTorProcess) { }
	~LoginHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		LoginCriteria criteria = LoginCriteria::FromJSON(request.GetParams().value());
		ValidateInput(criteria);

		try
		{
			auto response = m_pWalletManager->Login(criteria, m_pTorProcess);
			return request.BuildResult(response.ToJSON());
		}
		catch (const KeyChainException& e)
		{
			LOG_ERROR_F("Invalid password? {}", e.what());
			throw API_EXCEPTION(
				RPC::Errors::INVALID_PASSWORD.GetCode(),
				"Password is invalid"
			);
		}
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	void ValidateInput(const LoginCriteria& criteria) const
	{
		auto accounts = m_pWalletManager->GetAllAccounts();
		for (const auto& account : accounts)
		{
			if (account.ToLower() == criteria.GetUsername())
			{
				return;
			}
		}

		WALLET_ERROR_F("Failed to login as user {}. Username not found.", criteria.GetUsername());
		throw API_EXCEPTION_F(
			RPC::Errors::USER_DOESNT_EXIST.GetCode(),
			"Username {} not found",
			criteria.GetUsername()
		);
	}

	IWalletManagerPtr m_pWalletManager;
	TorProcess::Ptr m_pTorProcess;
};