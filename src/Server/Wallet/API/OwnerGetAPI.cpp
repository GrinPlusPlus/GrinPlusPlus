#include "OwnerGetAPI.h"
#include "SessionTokenUtil.h"
#include <Common/Compat.h>

#include <Net/Util/HTTPUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Exceptions/SessionTokenException.h>

/*
	GET /v1/wallet/owner/accounts
	GET /v1/wallet/owner/retrieve_outputs?refresh&show_spent&tx_id=x&tx_id=y
*/
int OwnerGetAPI::HandleGET(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager)
{
	if (action == "accounts")
	{
		Json::Value accountsNode(Json::arrayValue);
		auto accounts = walletManager.GetAllAccounts();
		for (const auto& account : accounts) {
			accountsNode.append(account);
		}

		return HTTPUtil::BuildSuccessResponse(pConnection, JsonUtil::WriteCondensed(accountsNode));
	}

	// GET /v1/wallet/owner/retrieve_outputs?show_spent&show_canceled
	if (action == "retrieve_outputs")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		const bool includeSpent = HTTPUtil::HasQueryParam(pConnection, "show_spent");
		const bool includeCanceled = HTTPUtil::HasQueryParam(pConnection, "show_canceled");

		std::vector<WalletOutputDTO> outputs = walletManager.GetWallet(token).Read()->GetOutputs(includeSpent, includeCanceled);

		Json::Value outputsJSON = Json::arrayValue;
		for (const WalletOutputDTO& output : outputs) {
			outputsJSON.append(output.ToJSON());
		}

		Json::Value response_json;
		response_json["outputs"] = outputsJSON;
		return HTTPUtil::BuildSuccessResponse(pConnection, JsonUtil::WriteCondensed(response_json));
	}

	return HTTPUtil::BuildBadRequestResponse(pConnection, "GET /v1/wallet/owner/" + action + " not Supported");
}