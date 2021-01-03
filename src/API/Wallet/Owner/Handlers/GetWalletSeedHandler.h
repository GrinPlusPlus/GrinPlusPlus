#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/GetSeedPhraseCriteria.h>
#include <optional>

class GetWalletSeedHandler : public RPCMethod
{
public:
	GetWalletSeedHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~GetWalletSeedHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		GetSeedPhraseCriteria criteria = GetSeedPhraseCriteria::FromJSON(request.GetParams().value());

		SecureString seedWords = m_pWalletManager->GetSeedWords(criteria);

		Json::Value result;
		result["wallet_seed"] = std::string(seedWords.begin(), seedWords.end());
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	IWalletManagerPtr m_pWalletManager;
};