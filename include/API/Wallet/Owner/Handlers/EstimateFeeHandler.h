#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/EstimateFeeCriteria.h>
#include <optional>

class EstimateFeeHandler : public RPCMethod
{
public:
	EstimateFeeHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~EstimateFeeHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		EstimateFeeCriteria criteria = EstimateFeeCriteria::FromJSON(request.GetParams().value());
		
		auto wallet = m_pWalletManager->GetWallet(criteria.GetToken());
		FeeEstimateDTO response = wallet.Read()->EstimateFee(criteria);

		return request.BuildResult(response.ToJSON());
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};