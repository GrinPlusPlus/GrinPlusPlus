#pragma once

#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Util/JsonUtil.h>
#include <P2P/P2PServer.h>
#include <BlockChain/BlockChain.h>

class SlateFromSlatepackMessageHandler : public RPCMethod
{
public:
	SlateFromSlatepackMessageHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~SlateFromSlatepackMessageHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		Json::Value result;
		result["Ok"] = "";
		
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};