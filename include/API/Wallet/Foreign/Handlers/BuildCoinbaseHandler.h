#pragma once

#include <API/Wallet/Foreign/Models/BuildCoinbaseCriteria.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Wallet/WalletManager.h>

class BuildCoinbaseHandler : RPCMethod
{
public:
	BuildCoinbaseHandler(IWalletManager& walletManager) : m_walletManager(walletManager) { }
	virtual ~BuildCoinbaseHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		BuildCoinbaseCriteria criteria = BuildCoinbaseCriteria::FromJSON(
			request.GetParams().value_or(Json::Value())
		);

		auto wallet = m_walletManager.GetWallet(criteria.GetToken());
		BuildCoinbaseResponse response = wallet.Write()->BuildCoinbase(criteria);

		return request.BuildResult(response.ToJSON());
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManager& m_walletManager;
};