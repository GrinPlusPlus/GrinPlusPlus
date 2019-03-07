#include "OwnerAPI.h"
#include "../WalletContext.h"
#include "../../RestUtil.h"

#include <Core/Util/JsonUtil.h>
#include <Wallet/SessionTokenException.h>

int OwnerAPI::Handler(mg_connection* pConnection, void* pWalletContext)
{
	WalletContext* pContext = (WalletContext*)pWalletContext;

	const std::string action = RestUtil::GetURIParam(pConnection, "/v1/wallet/owner/");
	const EHTTPMethod method = RestUtil::GetHTTPMethod(pConnection);

	try
	{
		if (method == EHTTPMethod::GET)
		{
			return HandleGET(pConnection, action, *pContext->m_pWalletManager, *pContext->m_pNodeClient);
		}
		else if (method == EHTTPMethod::POST)
		{
			return HandlePOST(pConnection, action, *pContext->m_pWalletManager, *pContext->m_pNodeClient);
		}
	}
	catch (const SessionTokenException&)
	{
		return RestUtil::BuildUnauthorizedResponse(pConnection, "session_token is missing or invalid.");
	}
	catch (const DeserializationException&)
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "Failed to deserialize one or more fields.");
	}
	catch (const std::exception&)
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "HTTPMethod not Supported");
}

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

	if (action == "logout")
	{
		const SessionToken token = GetSessionToken(pConnection);
		return Logout(pConnection, walletManager, token);
	}

	std::optional<Json::Value> requestBodyOpt = RestUtil::GetRequestBody(pConnection);
	if (!requestBodyOpt.has_value())
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "Request body not found.");
	}

	if (action == "issue_send_tx")
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
	/*
		// TODO: Since receive_tx requires session_token, maybe just leave foreign api/listener to front-end?
		cancel_tx
		post_tx
		repost
	*/

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
	const std::optional<std::string> minConfirmationsOpt = RestUtil::GetQueryParam(pConnection, "min_confirmations");
	const uint64_t minimumConfirmations = stoull(minConfirmationsOpt.value_or("10"));

	WalletSummary walletSummary = walletManager.GetWalletSummary(token, minimumConfirmations);
	
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
	try
	{
		Slate slate = Slate::FromJSON(json);

		const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message"); // TODO: Handle this

		if (walletManager.Receive(token, slate, messageOpt))
		{
			return RestUtil::BuildSuccessResponse(pConnection, slate.ToJSON().toStyledString());
		}
		else
		{
			return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
		}
	}
	catch (const std::exception&)
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerAPI::Finalize(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	try
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
	catch (const std::exception&)
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "Unknown error occurred.");
	}
}