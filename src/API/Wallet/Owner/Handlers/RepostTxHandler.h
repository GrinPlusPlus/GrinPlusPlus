#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Net/Tor/TorProcess.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/RepostTxCriteria.h>
#include <optional>

class RepostTxHandler : public RPCMethod
{
public:
	RepostTxHandler(const TorProcess::Ptr& pTorProcess, const IWalletManagerPtr& pWalletManager)
		: m_pTorProcess(pTorProcess), m_pWalletManager(pWalletManager) { }
	~RepostTxHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		RepostTxCriteria criteria = RepostTxCriteria::FromJSON(request.GetParams().value());

		const bool reposted = m_pWalletManager->RepostTx(criteria, m_pTorProcess);
		if (!reposted)
		{
			return request.BuildError(RPC::Errors::UNABLE_TO_POST);
		}

		Json::Value result;
		result["Ok"] = "";
		
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	TorProcess::Ptr m_pTorProcess;
	IWalletManagerPtr m_pWalletManager;
};