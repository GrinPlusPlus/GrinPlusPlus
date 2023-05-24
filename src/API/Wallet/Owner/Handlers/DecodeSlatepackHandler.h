#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Common/Util/FileUtil.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>
#include <API/Wallet/Owner/Models/DecodeSlatepackCriteria.h>

class DecodeSlatepackHandler : public RPCMethod
{
public:
	DecodeSlatepackHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~DecodeSlatepackHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
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

		DecodeSlatepackCriteria criteria = DecodeSlatepackCriteria::FromJSON(
			request.GetParams().value(),
			SlatepackDecryptor{ m_pWalletManager });

		Json::Value result;
		result["Ok"] = criteria.GetSlatepackMessage().ToJSON();

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

	IWalletManagerPtr m_pWalletManager;
};