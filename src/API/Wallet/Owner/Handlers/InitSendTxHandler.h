#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Wallet/Models/Slatepack/Armor.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorConnection.h>
#include <Net/Tor/TorProcess.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <Common/Util/FileUtil.h>
#include <API/Wallet/Owner/Models/Errors.h>
#include <API/Wallet/Owner/Models/SendResponse.h>
#include <API/Wallet/Foreign/Clients/CheckVersionClient.h>
#include <API/Wallet/Foreign/Clients/ReceiveTxClient.h>
#include <optional>

class InitSendTxHandler : public RPCMethod
{
public:
	InitSendTxHandler(const TorProcess::Ptr& pTorProcess, const IWalletManagerPtr& pWalletManager)
		: m_pTorProcess(pTorProcess), m_pWalletManager(pWalletManager) { }
	~InitSendTxHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}

		SendCriteria criteria = SendCriteria::FromJSON(request.GetParams().value());

		auto wallet = m_pWalletManager->GetWallet(criteria.GetToken());
		SlatepackAddress sender_address = wallet.Read()->GetSlatepackAddress();
		std::vector<SlatepackAddress> recipients = GetRecipients(criteria);

		if (!recipients.empty()) {
			criteria.SetSlateVersion(GetSlateVersion(recipients.front().ToTorAddress()));
		}

		Slate slate = m_pWalletManager->Send(criteria);
		LOG_INFO_F("Creating slate: {}", slate.ToJSON().toStyledString());

		SendResponse::EStatus status = SendResponse::EStatus::SENT;
		std::string slatepack = Armor::Pack(sender_address, slate, recipients);

		return request.BuildResult(SendResponse(status, slate, slatepack).ToJSON());
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	std::vector<SlatepackAddress> GetRecipients(const SendCriteria& criteria) const noexcept
	{
		std::vector<SlatepackAddress> recipients;

		if (criteria.GetAddress().has_value()) {
			try
			{
				SlatepackAddress slatepack_address = SlatepackAddress::Parse(criteria.GetAddress().value());
				recipients.emplace_back(std::move(slatepack_address));
			}
			catch (std::exception&) { }
		}
	
		return recipients;
	}

	uint16_t GetSlateVersion(const TorAddress& torAddress) const
	{
		TorConnectionPtr pTorConnection = m_pTorProcess->Connect(torAddress);
		if (pTorConnection == nullptr) {
			return MAX_SLATE_VERSION;
		}

		CheckVersionClient::Response version_response = CheckVersionClient::Call(*pTorConnection);
		if (!version_response.error.empty()) {
			return MAX_SLATE_VERSION;
		}

		if (version_response.slate_version < MIN_SLATE_VERSION) {
			throw RPC_EXCEPTION(RPC::Errors::SLATE_VERSION_MISMATCH.GetMsg(), std::nullopt);
		}

		return version_response.slate_version;
	}

	TorProcess::Ptr m_pTorProcess;
	IWalletManagerPtr m_pWalletManager;
};