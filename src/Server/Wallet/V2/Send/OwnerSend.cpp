#include "OwnerSend.h"

#include <Net/Tor/TorManager.h>
#include <Net/Tor/TorAddressParser.h>

OwnerSend::OwnerSend(const Config& config, IWalletManager& walletManager)
	: m_config(config), m_walletManager(walletManager)
{

}

RPC::Response OwnerSend::Send(mg_connection* pConnection, RPC::Request& request)
{
	if (!request.GetParams().has_value())
	{
		throw DESERIALIZATION_EXCEPTION("params missing");
	}

	const Json::Value& json = request.GetParams().value();
	const uint64_t amount = JsonUtil::GetRequiredUInt64(json, "amount");
	const uint64_t feeBase = JsonUtil::GetRequiredUInt64(json, "fee_base");
	const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");
	const uint8_t numOutputs = (uint8_t)json.get("change_outputs", Json::Value(1)).asUInt();
	const Json::Value selectionStrategyJSON = JsonUtil::GetRequiredField(json, "selection_strategy");
	const SelectionStrategyDTO selectionStrategy = SelectionStrategyDTO::FromJSON(selectionStrategyJSON);

	const std::optional<std::string> addressOpt = JsonUtil::GetStringOpt(json, "address");
	std::optional<TorConnection> torConnectionOpt = ConnectViaTor(addressOpt);

	std::unique_ptr<Slate> pSlate = m_walletManager.Send(token, amount, feeBase, addressOpt, messageOpt, selectionStrategy, numOutputs);
	if (pSlate != nullptr)
	{
		// TODO: Send to address
		if (torConnectionOpt.has_value())
		{
			Json::Value params;
			params.append(pSlate->ToJSON());
			params.append(Json::nullValue); // Account path not currently supported
			params.append(messageOpt.has_value() ? messageOpt.value() : Json::nullValue);
			RPC::Request receiveTxRequest = RPC::Request::BuildRequest("receive_tx", params);
			RPC::Response receiveTxResponse = receiveTxRequest.value().Send(receiveTxRequest);

			if (receiveTxResponse.GetError().has_value())
			{

			}
		}

		Json::Value result;
		result["Slate"] = pSlate->ToJSON();
		return RPC::Response::BuildResult(request.GetId(), result);
	}
	else
	{
		return RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, "Unknown error occurred.");
	}
}

std::optional<TorConnection> OwnerSend::EstablishConnection(const std::optional<std::string>& addressOpt) const
{
	if (addressOpt.has_value())
	{
		const std::optional<TorAddress> torAddress = TorAddressParser::Parse(addressOpt.value());
		if (torAddress.has_value())
		{
			std::unique_ptr<TorConnection> pTorConnection = TorManager::GetInstance(m_config.GetTorConfig()).Connect(torAddress.value());
			if (pTorConnection == nullptr)
			{
				throw HTTP_EXCEPTION("Failed to establish TOR connection.");
			}

			if (!pTorConnection->CheckVersion())
			{
				throw HTTP_EXCEPTION("Error occurred during check_version.");
			}

			return std::make_optional<TorConnection>(*pTorConnection);
		}
	}

	return nullptr;
}