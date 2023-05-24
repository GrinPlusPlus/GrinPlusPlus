#pragma once

#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Core/Util/JsonUtil.h>
#include <P2P/P2PServer.h>
#include <BlockChain/BlockChain.h>
#include <API/Wallet/Owner/Models/SlateFromSlatepackMessageCriteria.h>

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

		struct SlatepackDecryptor : public ISlatepackDecryptor
		{
			SlatepackDecryptor(const IWalletManagerPtr& pWalletManager_)
				: pWalletManager(pWalletManager_) { }

			IWalletManagerPtr pWalletManager;

			SlatepackMessage Decrypt(const SessionToken& token, const std::string& armored) const final
			{
				return pWalletManager->GetWallet(token).Read()->DecryptSlatepack(armored);
			}
		};

		SlateFromSlatepackMessageCriteria criteria = SlateFromSlatepackMessageCriteria::FromJSON(
														request.GetParams().value(),
														SlatepackDecryptor{ m_pWalletManager });
		
		Json::Value result;
		result["Ok"] = criteria.GetSlate().ToJSON();
		
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManagerPtr m_pWalletManager;
};