#pragma once

#include <Core/Config.h>

#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Util/JsonUtil.h>
#include <P2P/P2PServer.h>
#include <BlockChain/BlockChain.h>
#include <API/Wallet/Owner/Models/GetStoredTxCriteria.h>
#include <Wallet/WalletManager.h>

class GetStoredTxHandler : public RPCMethod
{
public:
	GetStoredTxHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~GetStoredTxHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		GetStoredTxCriteria criteria = GetStoredTxCriteria::FromJSON(request.GetParams().value());

		auto wallet = m_pWalletManager->GetWallet(criteria.GetToken());
		const Slate slate = wallet.Read()->GetSlate(criteria.GetTxSlateId());
		
		Json::Value result;
		result["Ok"] = slate.ToJSON();

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};