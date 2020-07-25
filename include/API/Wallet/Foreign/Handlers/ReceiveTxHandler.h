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
	ReceiveTxHandler(IWalletManager& walletManager, const SessionToken& token)
		: m_walletManager(walletManager), m_token(token) { }
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

			return request.BuildResult(ReceiveTxResponse(std::move(receivedSlate)));
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
};