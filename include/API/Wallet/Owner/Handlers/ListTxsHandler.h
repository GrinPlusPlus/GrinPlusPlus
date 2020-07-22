#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/ListTxsCriteria.h>
#include <optional>

class ListTxsHandler : public RPCMethod
{
public:
	ListTxsHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~ListTxsHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		ListTxsCriteria criteria = ListTxsCriteria::FromJSON(request.GetParams().value());

		const std::vector<WalletTxDTO> txs = m_pWalletManager->GetTransactions(criteria);

		Json::Value txsJson;
		for (const WalletTxDTO& tx : txs)
		{
			txsJson.append(tx.ToJSON());
		}

		Json::Value result;
		result["txs"] = txsJson;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	IWalletManagerPtr m_pWalletManager;
};