#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Common/Util/FileUtil.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <optional>

class ReceiveHandler : public RPCMethod
{
public:
	ReceiveHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~ReceiveHandler() = default;

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

		ReceiveCriteria criteria = ReceiveCriteria::FromJSON(
			request.GetParams().value(),
			SlatepackDecryptor{ m_pWalletManager }
		);

		SlatepackAddress sender;
		if (criteria.GetSlatepack().has_value()) {
			sender = criteria.GetSlatepack().value().m_sender;
		}

		WALLET_INFO_F("Receiving slatepack from {}", sender.IsNull() ? "unknown sender" : sender.ToString());

		Slate slate = m_pWalletManager->Receive(criteria);

		Json::Value result;
		result["status"] = "RECEIVED";

		SlatepackAddress address = m_pWalletManager->GetWallet(criteria.GetToken()).Read()->GetSlatepackAddress();

		std::vector<SlatepackAddress> recipients;
		if (!sender.IsNull()) {
			recipients.push_back(sender);
		}

		result["slatepack"] = Armor::Pack(address, slate, recipients);
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

	IWalletManagerPtr m_pWalletManager;
};