#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class GetAddressSecretKeyHandler : public RPCMethod
{
public:
	GetAddressSecretKeyHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~GetAddressSecretKeyHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		Json::Value tokenJson = JsonUtil::GetRequiredField(request.GetParams().value(), "session_token");
		SessionToken token = SessionToken::FromBase64(tokenJson.asString());

		ed25519_secret_key_t secretKey = m_pWalletManager->GetAddressSecretKey(token);
		
		Json::Value response;
		response["Ok"] = HexUtil::ConvertToHex(secretKey.vec());
		
		return request.BuildResult(response);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};