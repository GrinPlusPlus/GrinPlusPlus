#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Common/Logger.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class LogoutHandler : public RPCMethod
{
public:
	LogoutHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~LogoutHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		Json::Value tokenJson = JsonUtil::GetRequiredField(request.GetParams().value(), "session_token");
		SessionToken token = SessionToken::FromBase64(tokenJson.asString());

		m_pWalletManager->Logout(token);

		return request.BuildResult(Json::Value());
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};