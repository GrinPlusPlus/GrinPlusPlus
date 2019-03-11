#include "OwnerAPI.h"
#include "../WalletContext.h"
#include "../../RestUtil.h"

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionTokenException.h>

/*
	GET /v1/wallet/owner/node_height
	GET /v1/wallet/owner/retrieve_summary_info?min_confirmations=10
	GET /v1/wallet/owner/retrieve_outputs?refresh&show_spent&tx_id=x&tx_id=y
	GET /v1/wallet/owner/retrieve_txs?refresh&id=x
	GET /v1/wallet/owner/retrieve_stored_tx?id=x
*/
int OwnerAPI::HandleGET(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient)
{
	// GET /v1/wallet/owner/node_height
	if (action == "node_height")
	{
		Json::Value rootJSON;
		rootJSON["height"] = nodeClient.GetChainHeight();
		return RestUtil::BuildSuccessResponse(pConnection, rootJSON.toStyledString());
	}

	// GET /v1/wallet/owner/retrieve_summary_info?min_confirmations=10
	if (action == "retrieve_summary_info")
	{
		const SessionToken token = GetSessionToken(pConnection);
		return RetrieveSummaryInfo(pConnection, walletManager, token);
	}

	// GET /v1/wallet/owner/retrieve_outputs?refresh&show_spent&tx_id=x&tx_id=y
	if (action == "retrieve_outputs")
	{
		// TODO: Implement
	}

	// GET /v1/wallet/owner/retrieve_txs?refresh&id=x
	if (action == "retrieve_txs")
	{
		// TODO: Implement
	}

	// GET /v1/wallet/owner/retrieve_stored_tx?id=x
	if (action == "retrieve_stored_tx")
	{
		// TODO: Implement
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "GET /v1/wallet/owner/" + action + " not Supported");
}

int OwnerAPI::HandlePOST(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient)
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
		const SessionToken token = GetSessionToken(pConnection);
		return Logout(pConnection, walletManager, token);
	}
	else if (action == "update_wallet")
	{
		const SessionToken token = GetSessionToken(pConnection);
		return UpdateWallet(pConnection, walletManager, token);
	}

	std::optional<Json::Value> requestBodyOpt = RestUtil::GetRequestBody(pConnection);
	if (!requestBodyOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "Request body not found.");
	}

	if (action == "restore_wallet")
	{
		return RestoreWallet(pConnection, walletManager, requestBodyOpt.value());
	}
	else if (action == "issue_send_tx")
	{
		const SessionToken token = GetSessionToken(pConnection);
		return Send(pConnection, walletManager, token, requestBodyOpt.value());
	}
	else if (action == "receive_tx")
	{
		const SessionToken token = GetSessionToken(pConnection);
		return Receive(pConnection, walletManager, token, requestBodyOpt.value());
	}
	else if (action == "finalize_tx")
	{
		const SessionToken token = GetSessionToken(pConnection);
		return Finalize(pConnection, walletManager, token, requestBodyOpt.value());
	}
	else if (action == "cancel_tx")
	{
		// TODO: Implement
	}
	else if (action == "post_tx")
	{
		const SessionToken token = GetSessionToken(pConnection);
		return PostTx(pConnection, nodeClient, token, requestBodyOpt.value());
	}
	else if (action == "repost")
	{
		// TODO: Implement
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "POST /v1/wallet/owner/" + action + " not Supported");
}

SessionToken OwnerAPI::GetSessionToken(mg_connection* pConnection)
{
	const std::optional<std::string> sessionTokenOpt = RestUtil::GetHeaderValue(pConnection, "session_token");
	if (!sessionTokenOpt.has_value())
	{
		throw SessionTokenException();
	}

	try
	{
		return SessionToken::FromBase64(sessionTokenOpt.value());
	}
	catch (const DeserializationException&)
	{
		throw SessionTokenException();
	}
}

// GET /v1/wallet/owner/retrieve_summary_info?min_confirmations=10
int OwnerAPI::RetrieveSummaryInfo(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	WalletSummary walletSummary = walletManager.GetWalletSummary(token);
	
	return RestUtil::BuildSuccessResponse(pConnection, walletSummary.ToJSON().toStyledString());
}

int OwnerAPI::CreateWallet(mg_connection* pConnection, IWalletManager& walletManager)
{
	// TODO: Use MG auth handler?
	const std::optional<std::string> usernameOpt = RestUtil::GetHeaderValue(pConnection, "username");
	if (!usernameOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "username missing.");
	}

	const std::optional<std::string> passwordOpt = RestUtil::GetHeaderValue(pConnection, "password");
	if (!passwordOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "password missing.");
	}

	std::optional<std::pair<SecureString, SessionToken>> walletOpt = walletManager.InitializeNewWallet(usernameOpt.value(), SecureString(passwordOpt.value()));
	if (walletOpt.has_value())
	{
		Json::Value responseJSON;
		responseJSON["wallet_seed"] = std::string(walletOpt.value().first);
		responseJSON["session_token"] = std::string(walletOpt.value().second.ToBase64());
		return RestUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerAPI::Login(mg_connection* pConnection, IWalletManager& walletManager)
{
	// TODO: Use MG auth handler?
	const std::optional<std::string> usernameOpt = RestUtil::GetHeaderValue(pConnection, "username");
	if (!usernameOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "username missing");
	}

	const std::optional<std::string> passwordOpt = RestUtil::GetHeaderValue(pConnection, "password");
	if (!passwordOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "password missing");
	}

	std::unique_ptr<SessionToken> pSessionToken = walletManager.Login(usernameOpt.value(), SecureString(passwordOpt.value()));
	if (pSessionToken != nullptr)
	{
		Json::Value responseJSON;
		responseJSON["session_token"] = pSessionToken->ToBase64();
		return RestUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
	}
	else
	{
		return RestUtil::BuildUnauthorizedResponse(pConnection, "Invalid username/password");
	}
}

int OwnerAPI::Logout(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	walletManager.Logout(token);

	return RestUtil::BuildSuccessResponse(pConnection, "");
}

int OwnerAPI::RestoreWallet(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json)
{
	// TODO: Use MG auth handler?
	const std::optional<std::string> usernameOpt = RestUtil::GetHeaderValue(pConnection, "username");
	if (!usernameOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "username missing.");
	}

	const std::optional<std::string> passwordOpt = RestUtil::GetHeaderValue(pConnection, "password");
	if (!passwordOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "password missing.");
	}

	const Json::Value walletWordsJSON = JsonUtil::GetRequiredField(json, "wallet_seed");

	const std::string username = usernameOpt.value();
	const SecureString password(passwordOpt.value());
	const SecureString walletWords(walletWordsJSON.asString());

	std::optional<SessionToken> tokenOpt = walletManager.Restore(username, password, walletWords);
	if (tokenOpt.has_value())
	{
		Json::Value responseJSON;
		responseJSON["session_token"] = std::string(tokenOpt.value().ToBase64());
		return RestUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerAPI::UpdateWallet(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	if (walletManager.CheckForOutputs(token))
	{
		return RestUtil::BuildSuccessResponse(pConnection, "");
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "CheckForOutputs failed");
	}
}

int OwnerAPI::Send(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	const Json::Value amountJSON = json.get("amount", Json::nullValue);
	if (amountJSON == Json::nullValue || !amountJSON.isUInt64())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "amount missing");
	}

	const Json::Value feeBaseJSON = json.get("fee_base", Json::nullValue);
	if (feeBaseJSON == Json::nullValue || !feeBaseJSON.isUInt64())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "fee_base missing");
	}

	const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");

	const std::optional<std::string> selectionStrategyOpt = JsonUtil::GetStringOpt(json, "selection_strategy");
	if (!selectionStrategyOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "selection_strategy missing");
	}

	std::unique_ptr<Slate> pSlate = walletManager.Send(token, amountJSON.asUInt64(), feeBaseJSON.asUInt64(), messageOpt, SelectionStrategy::FromString(selectionStrategyOpt.value()));
	if (pSlate != nullptr)
	{
		return RestUtil::BuildSuccessResponse(pConnection, pSlate->ToJSON().toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerAPI::Receive(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	Slate slate = Slate::FromJSON(json);

	const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message"); // TODO: Handle this

	if (walletManager.Receive(token, slate, messageOpt))
	{
		Json::StreamWriterBuilder builder;
		builder["indentation"] = ""; // Removes whitespaces
		const std::string output = Json::writeString(builder, slate.ToJSON());
		return RestUtil::BuildSuccessResponse(pConnection, output);
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerAPI::Finalize(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	Slate slate = Slate::FromJSON(json);

	std::unique_ptr<Transaction> pTransaction = walletManager.Finalize(token, slate);
	if (pTransaction != nullptr)
	{
		return RestUtil::BuildSuccessResponse(pConnection, pTransaction->ToJSON().toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerAPI::PostTx(mg_connection* pConnection, INodeClient& nodeClient, const SessionToken& token, const Json::Value& json)
{
	Transaction transaction = Transaction::FromJSON(json);

	if (nodeClient.PostTransaction(transaction))
	{
		return RestUtil::BuildSuccessResponse(pConnection, "");
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}