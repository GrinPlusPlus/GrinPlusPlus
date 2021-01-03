#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Net/Tor/TorProcess.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <Common/Util/FileUtil.h>
#include <optional>

class FinalizeHandler : public RPCMethod
{
public:
	FinalizeHandler(const TorProcess::Ptr& pTorProcess, const IWalletManagerPtr& pWalletManager)
		: m_pTorProcess(pTorProcess), m_pWalletManager(pWalletManager) { }
	~FinalizeHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		struct SlatepackDecryptor : public ISlatepackDecryptor
		{
			SlatepackDecryptor(const IWalletManagerPtr& pWalletManager_)
				: pWalletManager(pWalletManager_){ }

			IWalletManagerPtr pWalletManager;

			SlatepackMessage Decrypt(const SessionToken& token, const std::string& armored) const final
			{
				return pWalletManager->GetWallet(token).Read()->DecryptSlatepack(armored);
			}
		};

		FinalizeCriteria criteria = FinalizeCriteria::FromJSON(request.GetParams().value(), SlatepackDecryptor{ m_pWalletManager });

		Slate slate = m_pWalletManager->Finalize(criteria, m_pTorProcess);

		std::vector<SlatepackAddress> recipients;
		if (criteria.GetSlatepack().has_value() && !criteria.GetSlatepack().value().m_sender.IsNull()) {
			recipients.push_back(criteria.GetSlatepack().value().m_sender);
		}

		SlatepackAddress address = m_pWalletManager->GetWallet(criteria.GetToken()).Read()->GetSlatepackAddress();

		Json::Value result;
		result["status"] = "FINALIZED";
		result["slate"] = slate.ToJSON();
		result["slatepack"] = Armor::Pack(address, slate, recipients);
		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	TorProcess::Ptr m_pTorProcess;
	IWalletManagerPtr m_pWalletManager;
};