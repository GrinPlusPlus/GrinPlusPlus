#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Foreign/Models/ReceiveTxRequest.h>
#include <API/Wallet/Foreign/Models/ReceiveTxResponse.h>
#include <optional>

class ReceiveTxHandler : RPCMethod
{
public:
	ReceiveTxHandler(IWalletManager& walletManager, const SessionToken& token, const TorProcess::Ptr& pTorProcess)
		: m_walletManager(walletManager), m_token(token), m_pTorProcess(pTorProcess) { }
	virtual ~ReceiveTxHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		const std::optional<Json::Value>& paramsOpt = request.GetParams();
		try
		{
			ReceiveTxRequest receiveTxRequest = ReceiveTxRequest::FromJSON(
				paramsOpt.value_or(Json::Value(Json::nullValue))
			);

			ReceiveCriteria criteria(
				SessionToken(m_token),
				std::nullopt,
				Slate(receiveTxRequest.GetSlate()),
				std::nullopt
			);

			Slate receivedSlate = m_walletManager.Receive(criteria);
			
			// Update keychain index if m_reuseAddress is set false
			if (!m_walletManager.ShouldReuseAddresses())
			{		
				m_walletManager.RemoveCurrentTorListener(m_token, m_pTorProcess);

				KeyChainPath newPath = m_walletManager.IncreaseAddressKeyChainPathIndex(m_token);
				std::optional<TorAddress> torAddress = m_walletManager.AddTorListener(m_token, newPath, m_pTorProcess);
				m_walletManager.GetWallet(m_token).Write()->SetSlatepackAddress(torAddress.value().GetPublicKey());
			}
			
			RPC::Response response = request.BuildResult(ReceiveTxResponse(std::move(receivedSlate)));
			
			return response;
		}
		catch (const DeserializationException& e)
		{
			return request.BuildError(
				RPC::ErrorCode::INVALID_PARAMS,
				StringUtil::Format("Failed to deserialize slate: {}", e.what())
			);
		}
		catch (const APIException& e)
		{
			return request.BuildError(e.GetErrorCode(), e.what());
		}
		catch (std::exception& e)
		{
			return request.BuildError(RPC::ErrorCode::INTERNAL_ERROR, e.what());
		}
	}

	bool ContainsSecrets() const noexcept final { return false; }

private:
	IWalletManager& m_walletManager;
	SessionToken m_token;
	TorProcess::Ptr m_pTorProcess;
};