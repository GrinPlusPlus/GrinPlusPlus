#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <API/Wallet/Owner/Models/Errors.h>

class ListOutputsHandler : public RPCMethod
{
public:
	ListOutputsHandler(const IWalletManagerPtr& pWalletManager)
		: m_pWalletManager(pWalletManager) { }
	virtual ~ListOutputsHandler() = default;

	RPC::Response Handle(const RPC::Request& request) const final
	{
		if (!request.GetParams().has_value()) {
			return request.BuildError(RPC::Errors::PARAMS_MISSING);
		}
		
		const Json::Value& request_json = request.GetParams().value();
		SessionToken token = SessionToken::FromBase64(
			JsonUtil::GetRequiredString(request_json, "session_token")
		);

		const bool includeSpent = false;
		const bool includeCanceled = false;
		auto wallet = m_pWalletManager->GetWallet(token);
		std::vector<WalletOutputDTO> outputs = wallet.Read()->GetOutputs(includeSpent, includeCanceled);

		Json::Value outputs_json = Json::arrayValue;
		for (const WalletOutputDTO& output : outputs) {
			outputs_json.append(output.ToJSON());
		}

		Json::Value response_json;
		response_json["outputs"] = outputs_json;
		return request.BuildResult(response_json);
	}

	bool ContainsSecrets() const noexcept final { return true; }

private:
	IWalletManagerPtr m_pWalletManager;
};