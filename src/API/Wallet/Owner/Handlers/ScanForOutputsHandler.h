#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCMethod.h>

class ScanForOutputsHandler : public RPCMethod
{
public:
	ScanForOutputsHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		SessionToken token = SessionToken::FromBase64(
			JsonUtil::GetRequiredString(request.GetParams().value(), "session_token")
		);

		m_pWalletManager->CheckForOutputs(token, true);

		return request.BuildResult(Json::Value{});
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};