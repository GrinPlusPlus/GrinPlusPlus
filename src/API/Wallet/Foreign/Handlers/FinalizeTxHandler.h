#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Foreign/Models/FinalizeTxRequest.h>
#include <API/Wallet/Foreign/Models/FinalizeTxResponse.h>
#include <optional>

class FinalizeTxHandler : public RPCMethod
{
public:
	FinalizeTxHandler(IWalletManager& walletManager, const SessionToken& token, const TorProcess::Ptr& pTorProcess)
		: m_walletManager(walletManager), m_token(token), m_pTorProcess(pTorProcess) { }
	virtual ~FinalizeTxHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		const std::optional<Json::Value>& paramsOpt = request.GetParams();
		try
		{
			FinalizeTxRequest finalizeTxRequest = FinalizeTxRequest::FromJSON(
				paramsOpt.value_or(Json::Value(Json::nullValue))
			);

			FinalizeCriteria criteria(
				SessionToken(m_token),
				std::nullopt,
				Slate(finalizeTxRequest.GetSlate()),
				std::make_optional(PostMethodDTO(EPostMethod::STEM, std::nullopt))
			);

			Slate finalized_slate = m_walletManager.Finalize(criteria, m_pTorProcess);

			return request.BuildResult(FinalizeTxResponse(std::move(finalized_slate)));
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