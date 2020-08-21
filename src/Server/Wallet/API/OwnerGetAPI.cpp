#include "OwnerGetAPI.h"
#include "SessionTokenUtil.h"
#include <Common/Compat.h>

#include <Net/Util/HTTPUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Exceptions/SessionTokenException.h>

/*
	GET /v1/wallet/owner/node_height
	GET /v1/wallet/owner/retrieve_summary_info
	GET /v1/wallet/owner/retrieve_outputs?refresh&show_spent&tx_id=x&tx_id=y
	GET /v1/wallet/owner/retrieve_txs?refresh&id=x
	GET /v1/wallet/owner/retrieve_stored_tx?id=x
*/
int OwnerGetAPI::HandleGET(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient)
{
	if (action == "accounts")
	{
		Json::Value accountsNode(Json::arrayValue);
		auto accounts = walletManager.GetAllAccounts();
		for (const auto& account : accounts)
		{
			accountsNode.append(account);
		}

		return HTTPUtil::BuildSuccessResponse(pConnection, accountsNode.toStyledString());
	}

	if (action == "retrieve_mnemonic")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		Json::Value json;
		SecureString walletWords = walletManager.GetWallet(token).Read()->GetSeedWords();
		json["mnemonic"] = (std::string)walletWords;
		return HTTPUtil::BuildSuccessResponse(pConnection, json.toStyledString());
	}

	// GET /v1/wallet/owner/node_height
	if (action == "node_height")
	{
		return GetNodeHeight(pConnection, nodeClient);
	}

	// GET /v1/wallet/owner/retrieve_summary_info
	if (action == "retrieve_summary_info")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return RetrieveSummaryInfo(pConnection, walletManager, token);
	}

	// GET /v1/wallet/owner/retrieve_outputs?show_spent&show_canceled
	if (action == "retrieve_outputs")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return RetrieveOutputs(pConnection, walletManager, token);
	}

	// GET /v1/wallet/owner/retrieve_txs?refresh&id=x
	if (action == "retrieve_txs")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return RetrieveTransactions(pConnection, walletManager, token);
	}

	return HTTPUtil::BuildBadRequestResponse(pConnection, "GET /v1/wallet/owner/" + action + " not Supported");
}

// GET /v1/wallet/owner/node_height
int OwnerGetAPI::GetNodeHeight(mg_connection* pConnection, INodeClient& nodeClient)
{
	// TODO: Load from wallet instead, and return height of last refresh
	Json::Value rootJSON;
	rootJSON["height"] = nodeClient.GetChainHeight();
	return HTTPUtil::BuildSuccessResponse(pConnection, rootJSON.toStyledString());
}

// GET /v1/wallet/owner/retrieve_summary_info
int OwnerGetAPI::RetrieveSummaryInfo(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	WalletSummaryDTO walletSummary = walletManager.GetWallet(token).Read()->GetWalletSummary();

	return HTTPUtil::BuildSuccessResponse(pConnection, walletSummary.ToJSON().toStyledString());
}


// GET /v1/wallet/owner/retrieve_txs?refresh&id=x
int OwnerGetAPI::RetrieveTransactions(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	Json::Value rootJSON;

	const std::optional<std::string> txIdOpt = HTTPUtil::GetQueryParam(pConnection, "id");

	Json::Value transactionsJSON = Json::arrayValue;

	ListTxsCriteria criteria(token, std::nullopt, std::nullopt, {});

	auto wallet = walletManager.GetWallet(token);

	if (txIdOpt.has_value())
	{
		const uint64_t txId = std::stoull(txIdOpt.value());

		// TODO: Filter in walletManager for better performance
		const std::vector<WalletTxDTO> transactions = wallet.Read()->GetTransactions(criteria);
		for (const WalletTxDTO& transaction : transactions)
		{
			if (transaction.GetId() == txId)
			{
				transactionsJSON.append(transaction.ToJSON());
			}
		}
	}
	else
	{
		const std::vector<WalletTxDTO> transactions = wallet.Read()->GetTransactions(criteria);
		for (const WalletTxDTO& transaction : transactions)
		{
			transactionsJSON.append(transaction.ToJSON());
		}
	}

	rootJSON["transactions"] = transactionsJSON;
	return HTTPUtil::BuildSuccessResponse(pConnection, rootJSON.toStyledString());
}

// GET /v1/wallet/owner/retrieve_outputs?show_spent&show_canceled
int OwnerGetAPI::RetrieveOutputs(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	Json::Value rootJSON;

	Json::Value outputsJSON = Json::arrayValue;

	const bool includeSpent = HTTPUtil::HasQueryParam(pConnection, "show_spent");
	const bool includeCanceled = HTTPUtil::HasQueryParam(pConnection, "show_canceled");

	const std::vector<WalletOutputDTO> outputs = walletManager.GetWallet(token).Read()->GetOutputs(includeSpent, includeCanceled);
	for (const WalletOutputDTO& output : outputs)
	{
		outputsJSON.append(output.ToJSON());
	}

	rootJSON["outputs"] = outputsJSON;
	return HTTPUtil::BuildSuccessResponse(pConnection, rootJSON.toStyledString());
}