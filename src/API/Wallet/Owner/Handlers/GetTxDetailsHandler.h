#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/GetTxDetailsCriteria.h>
#include <API/Wallet/Owner/Models/ListTxsCriteria.h>

#include <unordered_set>
#include <optional>
#include <chrono>

class GetTxDetailsHandler : public RPCMethod
{
public:
	GetTxDetailsHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~GetTxDetailsHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		GetTxDetailsCriteria criteria = GetTxDetailsCriteria::FromJSON(request.GetParams().value());

		auto wallet = m_pWalletManager->GetWallet(criteria.GetToken());
		std::unordered_set<uint32_t> ids({criteria.GetTxId()});

		std::optional<std::chrono::system_clock::time_point> start_range = std::nullopt;
		std::optional<std::chrono::system_clock::time_point> end_range = std::nullopt;
		std::unordered_set<EWalletTxType> statuses;
		const std::vector<WalletTxDTO> txs = wallet.Read()->GetTransactions(ListTxsCriteria(criteria.GetToken(), ids, start_range, end_range, statuses));
		
		Json::Value txsJson;
		
		for (const WalletTxDTO& tx : txs)
		{
			txsJson = tx.ToJSON();
		}

		Json::Value result;
		result["Ok"] = txsJson;
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	IWalletManagerPtr m_pWalletManager;
};