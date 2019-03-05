#include "OwnerAPI.h"
#include "../WalletContext.h"
#include "../../RestUtil.h"

#include <Core/Util/JsonUtil.h>

int OwnerAPI::Handler(mg_connection* pConnection, void* pWalletContext)
{
	WalletContext* pContext = (WalletContext*)pWalletContext;

	const std::string action = RestUtil::GetURIParam(pConnection, "/v1/wallet/owner/");
	const EHTTPMethod method = RestUtil::GetHTTPMethod(pConnection);
	if (method == EHTTPMethod::GET)
	{
		/*
			retrieve_outputs
			retrieve_summary_info
			node_height
			retrieve_txs
			retrieve_stored_tx
		*/
	}
	else if (method == EHTTPMethod::POST)
	{
		if (action == "create_wallet")
		{
			return CreateWallet(pConnection, *pContext->m_pWalletManager);
		}
		else if (action == "login")
		{
			return Login(pConnection, *pContext->m_pWalletManager);
		}

		const std::optional<std::string> sessionTokenOpt = RestUtil::GetHeaderValue(pConnection, "session_token");
		if (!sessionTokenOpt.has_value())
		{
			return RestUtil::BuildUnauthorizedResponse(pConnection, "session_token missing");
		}

		const SessionToken token = SessionToken::FromBase64(sessionTokenOpt.value());

		if (action == "logout")
		{
			return Logout(pConnection, *pContext->m_pWalletManager, token);
		}

		std::optional<std::string> requestBodyOpt = RestUtil::GetRequestBody(pConnection);
		if (!requestBodyOpt.has_value())
		{
			return RestUtil::BuildBadRequestResponse(pConnection, "Request body not found.");
		}

		Json::Value json;
		Json::Reader reader;
		if (!reader.parse(requestBodyOpt.value(), json))
		{
			return RestUtil::BuildBadRequestResponse(pConnection, "Request body not valid JSON.");
		}

		if (action == "issue_send_tx")
		{
			return Send(pConnection, *pContext->m_pWalletManager, token, json);
		}
		else if (action == "receive_tx")
		{
			return Receive(pConnection, *pContext->m_pWalletManager, token, json);
		}
		/*
			// TODO: If receive_tx included, maybe just leave foreign api/listener to front-end?
			issue_send_tx
			finalize_tx
			cancel_tx
			post_tx
			repost
		*/
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "Not Supported");
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

	const SecureString seed = walletManager.InitializeNewWallet(usernameOpt.value(), SecureString(passwordOpt.value()));
	if (!seed.empty())
	{
		Json::Value responseJSON;
		responseJSON["wallet_seed"] = std::string(seed);
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

		const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message");

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