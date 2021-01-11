#pragma once

#include <Core/Config.h>
#include <Common/Secure.h>
#include <Common/Util/StringUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Net/Tor/TorProcess.h>
#include <API/Wallet/Owner/Models/RestoreWalletCriteria.h>
#include <API/Wallet/Owner/Models/LoginResponse.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class RestoreWalletHandler : public RPCMethod
{
public:
	RestoreWalletHandler(const IWalletManagerPtr& pWalletManager, const TorProcess::Ptr& pTorProcess)
		: m_pWalletManager(pWalletManager), m_pTorProcess(pTorProcess) { }
	~RestoreWalletHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		RestoreWalletCriteria criteria = RestoreWalletCriteria::FromJSON(request.GetParams().value());
		ValidateInput(criteria);

		auto response = m_pWalletManager->RestoreFromSeed(criteria, m_pTorProcess);

		return request.BuildResult(response.ToJSON());
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	void ValidateInput(const RestoreWalletCriteria& criteria) const
	{
		// TODO: Should we allow usernames to contain spaces or special characters?

		auto accounts = m_pWalletManager->GetAllAccounts();
		for (const auto& account : accounts)
		{
			if (account.ToLower() == criteria.GetUsername())
			{
				throw API_EXCEPTION_F(
					RPC::ErrorCode::INVALID_PARAMS,
					"Username {} already exists",
					criteria.GetUsername()
				);
			}
		}

		if (criteria.GetPassword().empty())
		{
			throw API_EXCEPTION(
				RPC::ErrorCode::INVALID_PARAMS,
				"Password cannot be empty"
			);
		}

		const size_t numWords = StringUtil::Split((const std::string&)criteria.GetSeedWords(), " ").size();
		if (numWords < 12 || numWords > 24 || numWords % 3 != 0)
		{
			throw API_EXCEPTION_F(
				RPC::ErrorCode::INVALID_PARAMS,
				"Invalid num_seed_words ({}). Only 12, 15, 18, 21, and 24 are supported.",
				numWords
			);
		}
	}

	IWalletManagerPtr m_pWalletManager;
	TorProcess::Ptr m_pTorProcess;
};