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
		auto wallet = m_pWalletManager->GetWallet(criteria.GetToken());

		Json::Value result;
		result["Ok"] = wallet.Write()->CancelTx(criteria.GetTxId()) ? "SUCCESS" : "ERROR";
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};