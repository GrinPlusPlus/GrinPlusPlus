#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class GetNewWalletAddressHandler : public RPCMethod
{
public:
	GetNewWalletAddressHandler(const IWalletManagerPtr& pWalletManager, const TorProcess::Ptr& pTorProcess)
		: m_pWalletManager(pWalletManager), m_pTorProcess(pTorProcess) { }
	virtual ~GetNewWalletAddressHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		Json::Value tokenJson = JsonUtil::GetRequiredField(request.GetParams().value(), "session_token");
		SessionToken token = SessionToken::FromBase64(tokenJson.asString());

		KeyChainPath newPath = m_pWalletManager->IncreaseAddressKeyChainPathIndex(token);
		std::optional<TorAddress> torAddress = m_pWalletManager->AddTorListener(token, newPath, m_pTorProcess);
		m_pWalletManager->GetWallet(token).Write()->SetSlatepackAddress(torAddress.value().GetPublicKey());
		
		auto address = m_pWalletManager->GetWallet(token).Read()->GetSlatepackAddress();

		return request.BuildResult(address.ToJSON());
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
	TorProcess::Ptr m_pTorProcess;
};