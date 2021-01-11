#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCMethod.h>

class ListWalletsHandler : public RPCMethod
{
public:
	ListWalletsHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~ListWalletsHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		Json::Value wallets_json(Json::arrayValue);
		auto wallet_names = m_pWalletManager->GetAllAccounts();
		for (const auto& wallet_name : wallet_names) {
			wallets_json.append(wallet_name);
		}

		Json::Value response_json;
		response_json["wallets"] = wallets_json;
		return request.BuildResult(response_json);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};