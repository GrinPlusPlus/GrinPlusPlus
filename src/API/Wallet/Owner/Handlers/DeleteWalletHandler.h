#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/DeleteWalletCriteria.h>
#include <optional>

class DeleteWalletHandler : public RPCMethod
{
public:
	DeleteWalletHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~DeleteWalletHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		DeleteWalletCriteria criteria = DeleteWalletCriteria::FromJSON(request.GetParams().value());

		m_pWalletManager->DeleteWallet(criteria.GetUsername(), criteria.GetPassword());

		Json::Value result;
		result["status"] = "SUCCESS";
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	IWalletManagerPtr m_pWalletManager;
};