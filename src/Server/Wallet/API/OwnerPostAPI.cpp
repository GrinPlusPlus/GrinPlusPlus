#include "OwnerPostAPI.h"
#include "SessionTokenUtil.h"

#include <Net/Util/HTTPUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Core/Exceptions/WalletException.h>
#include <Wallet/Exceptions/SessionTokenException.h>
#include <Wallet/Exceptions/InvalidMnemonicException.h>

OwnerPostAPI::OwnerPostAPI(const Config& config)
	: m_config(config)
{

}

int OwnerPostAPI::HandlePOST(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager)
{
	if (action == "create_wallet")
	{
		return CreateWallet(pConnection, walletManager);
	}
	else if (action == "login")
	{
		return Login(pConnection, walletManager);
	}
	else if (action == "logout")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return Logout(pConnection, walletManager, token);
	}
	else if (action == "update_wallet")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return UpdateWallet(pConnection, walletManager, token);
	}
	else if (action == "cancel_tx")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return Cancel(pConnection, walletManager, token);
	}
	else if (action == "repost_tx")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return Repost(pConnection, walletManager, token);
	}

	std::optional<Json::Value> requestBodyOpt = HTTPUtil::GetRequestBody(pConnection);
	if (!requestBodyOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "Request body not found.");
	}

	if (action == "restore_wallet")
	{
		return RestoreWallet(pConnection, walletManager, requestBodyOpt.value());
	}
	else if (action == "issue_send_tx")
	{
		return Send(pConnection, walletManager, requestBodyOpt.value());
	}
	else if (action == "receive_tx")
	{
		return Receive(pConnection, walletManager, requestBodyOpt.value());
	}
	else if (action == "finalize_tx")
	{
		return Finalize(pConnection, walletManager, requestBodyOpt.value());
	}
	else if (action == "estimate_fee")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return EstimateFee(pConnection, walletManager, token, requestBodyOpt.value());
	}

	return HTTPUtil::BuildBadRequestResponse(pConnection, "POST /v1/wallet/owner/" + action + " not Supported");
}

int OwnerPostAPI::CreateWallet(mg_connection* pConnection, IWalletManager& walletManager)
{
	const std::optional<std::string> usernameOpt = HTTPUtil::GetHeaderValue(pConnection, "username");
	if (!usernameOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "username missing.");
	}

	const std::optional<std::string> passwordOpt = HTTPUtil::GetHeaderValue(pConnection, "password");
	if (!passwordOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "password missing.");
	}

	std::pair<SecureString, SessionToken> wallet = walletManager.InitializeNewWallet(usernameOpt.value(), SecureString(passwordOpt.value()));

	Json::Value responseJSON;
	responseJSON["wallet_seed"] = std::string(wallet.first);
	responseJSON["session_token"] = std::string(wallet.second.ToBase64());

	return HTTPUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
}

int OwnerPostAPI::Login(mg_connection* pConnection, IWalletManager& walletManager)
{
	const std::optional<std::string> usernameOpt = HTTPUtil::GetHeaderValue(pConnection, "username");
	if (!usernameOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "username missing");
	}

	const std::optional<std::string> passwordOpt = HTTPUtil::GetHeaderValue(pConnection, "password");
	if (!passwordOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "password missing");
	}

	try
	{
		SessionToken sessionToken = walletManager.Login(usernameOpt.value(), SecureString(passwordOpt.value()));
		Json::Value responseJSON;
		responseJSON["session_token"] = sessionToken.ToBase64();

		const std::optional<TorAddress> torAddressOpt = walletManager.GetTorAddress(sessionToken);
		if (torAddressOpt.has_value())
		{
			responseJSON["tor_address"] = torAddressOpt.value().ToString();
		}

		responseJSON["listener_port"] = walletManager.GetListenerPort(sessionToken);

		return HTTPUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
	}
	catch (const WalletException&)
	{
		return HTTPUtil::BuildUnauthorizedResponse(pConnection, "Invalid username/password");
	}
}

int OwnerPostAPI::Logout(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	walletManager.Logout(token);

	return HTTPUtil::BuildSuccessResponse(pConnection, "");
}

int OwnerPostAPI::RestoreWallet(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json)
{
	const std::optional<std::string> usernameOpt = HTTPUtil::GetHeaderValue(pConnection, "username");
	if (!usernameOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "username missing.");
	}

	const std::optional<std::string> passwordOpt = HTTPUtil::GetHeaderValue(pConnection, "password");
	if (!passwordOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "password missing.");
	}

	const Json::Value walletWordsJSON = JsonUtil::GetRequiredField(json, "wallet_seed");

	const std::string username = usernameOpt.value();
	const SecureString password(passwordOpt.value());
	const SecureString walletWords(walletWordsJSON.asString());

	try
	{
		std::optional<SessionToken> tokenOpt = walletManager.RestoreFromSeed(username, password, walletWords);
		if (tokenOpt.has_value())
		{
			Json::Value responseJSON;
			responseJSON["session_token"] = std::string(tokenOpt.value().ToBase64());

			return HTTPUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
		}
		else
		{
			return HTTPUtil::BuildInternalErrorResponse(pConnection, "Username already exists.");
		}
	}
	catch (const InvalidMnemonicException&)
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "Mnemonic Invalid");
	}
}

int OwnerPostAPI::UpdateWallet(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	std::optional<std::string> fromGenesis = HTTPUtil::GetQueryParam(pConnection, "fromGenesis");
	walletManager.CheckForOutputs(token, fromGenesis.has_value());
	return HTTPUtil::BuildSuccessResponse(pConnection, "");
}

int OwnerPostAPI::Send(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json)
{
	Slate slate = walletManager.Send(SendCriteria::FromJSON(json));
	
	return HTTPUtil::BuildSuccessResponseJSON(pConnection, slate.ToJSON());
}

int OwnerPostAPI::Receive(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json)
{
	Slate receivedSlate = walletManager.Receive(ReceiveCriteria::FromJSON(json));
	
	return HTTPUtil::BuildSuccessResponseJSON(pConnection, receivedSlate.ToJSON());
}

int OwnerPostAPI::Finalize(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json)
{
	FinalizeCriteria finalizeCriteria = FinalizeCriteria::FromJSON(json);

	Slate finalizedSlate = walletManager.Finalize(finalizeCriteria);

	return HTTPUtil::BuildSuccessResponseJSON(pConnection, finalizedSlate.ToJSON());
}

int OwnerPostAPI::Repost(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	std::optional<std::string> idOpt = HTTPUtil::GetQueryParam(pConnection, "id");
	if (idOpt.has_value())
	{
		const uint32_t id = std::stoul(idOpt.value());
		if (walletManager.RepostByTxId(token, id))
		{
			return HTTPUtil::BuildSuccessResponse(pConnection, "");
		}
		else
		{
			return HTTPUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
		}
	}
	else
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "id missing");
	}
}

int OwnerPostAPI::Cancel(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	std::optional<std::string> idOpt = HTTPUtil::GetQueryParam(pConnection, "id");
	if (idOpt.has_value())
	{
		const uint32_t id = std::stoul(idOpt.value());
		walletManager.CancelByTxId(token, id);
		return HTTPUtil::BuildSuccessResponse(pConnection, "");
	}
	else
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "id missing");
	}
}

int OwnerPostAPI::EstimateFee(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	const std::optional<uint64_t> amountOpt = JsonUtil::GetUInt64Opt(json, "amount");
	if (!amountOpt.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "amount missing");
	}

	const Json::Value feeBaseJSON = json.get("fee_base", Json::nullValue);
	if (feeBaseJSON == Json::nullValue || !feeBaseJSON.isUInt64())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "fee_base missing");
	}

	const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");

	const std::optional<Json::Value> selectionStrategyJSON = JsonUtil::GetOptionalField(json, "selection_strategy");
	if (!selectionStrategyJSON.has_value())
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "selection_strategy missing");
	}

	const SelectionStrategyDTO selectionStrategy = SelectionStrategyDTO::FromJSON(selectionStrategyJSON.value());
	const uint8_t numOutputs = (uint8_t)json.get("change_outputs", Json::Value(1)).asUInt();
	const FeeEstimateDTO estimatedFee = walletManager.EstimateFee(token, amountOpt.value(), feeBaseJSON.asUInt64(), selectionStrategy, numOutputs);

	return HTTPUtil::BuildSuccessResponseJSON(pConnection, estimatedFee.ToJSON());
}