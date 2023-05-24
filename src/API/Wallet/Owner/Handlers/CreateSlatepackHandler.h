#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Common/Util/FileUtil.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/CreateSlatepackCriteria.h>
#include <optional>

class CreateSlatepackHandler : public RPCMethod
{
public:
	CreateSlatepackHandler(const TorProcess::Ptr& pTorProcess, const IWalletManagerPtr& pWalletManager)
		: m_pTorProcess(pTorProcess), m_pWalletManager(pWalletManager) { }
	virtual ~CreateSlatepackHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value())
		{
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		CreateSlatepackCriteria criteria = CreateSlatepackCriteria::FromJSON(request.GetParams().value());

		auto wallet = m_pWalletManager->GetWallet(criteria.GetToken());
		SlatepackAddress sender_address = wallet.Read()->GetSlatepackAddress();
		std::vector<SlatepackAddress> recipients = GetRecipients(criteria);

		Slate slate = criteria.GetSlate();
		
		Json::Value result;
		result["Ok"] = Armor::Pack(sender_address, slate, recipients);

		return request.BuildResult(result);
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	std::vector<SlatepackAddress> GetRecipients(const CreateSlatepackCriteria& criteria) const noexcept
	{
		std::vector<SlatepackAddress> recipients;

		if (criteria.GetAddress().has_value()) {
			try
			{
				SlatepackAddress slatepack_address = SlatepackAddress::Parse(criteria.GetAddress().value());
				recipients.emplace_back(std::move(slatepack_address));
			}
			catch (std::exception&) {}
		}

		return recipients;
	}

	TorProcess::Ptr m_pTorProcess;
	IWalletManagerPtr m_pWalletManager;
};