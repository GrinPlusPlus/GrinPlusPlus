#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/CancelTxCriteria.h>
#include <optional>

class CancelTxHandler : public RPCMethod
{
public:
	CancelTxHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~CancelTxHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		CancelTxCriteria criteria = CancelTxCriteria::FromJSON(request.GetParams().value());

		m_pWalletManager->CancelByTxId(criteria.GetToken(), criteria.GetTxId());

		Json::Value result;
		result["status"] = "SUCCESS";
		return request.BuildResult(result);
	}

private:
	IWalletManagerPtr m_pWalletManager;
};