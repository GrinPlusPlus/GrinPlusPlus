#include "OwnerGetAPI.h"
#include "../../RestUtil.h"
#include "SessionTokenUtil.h"

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
		Json::Value accountsNode;
		std::vector<std::string> accounts = walletManager.GetAllAccounts();
		for (std::string& account : accounts)
		{
			accountsNode.append(account);
		}
		return RestUtil::BuildSuccessResponse(pConnection, accountsNode.toStyledString());
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

	// GET /v1/wallet/owner/retrieve_outputs?refresh&show_spent&tx_id=x&tx_id=y
	if (action == "retrieve_outputs")
	{
		// TODO: Implement
	}

	// GET /v1/wallet/owner/retrieve_txs?refresh&id=x
	if (action == "retrieve_txs")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return RetrieveTransactions(pConnection, walletManager, token);
	}

	// GET /v1/wallet/owner/retrieve_stored_tx?id=x
	if (action == "retrieve_stored_tx")
	{
		// TODO: Implement
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "GET /v1/wallet/owner/" + action + " not Supported");
}

// GET /v1/wallet/owner/node_height
int OwnerGetAPI::GetNodeHeight(mg_connection* pConnection, INodeClient& nodeClient)
{
	// TODO: Load from wallet instead, and return height of last refresh
	Json::Value rootJSON;
	rootJSON["height"] = nodeClient.GetChainHeight();
	return RestUtil::BuildSuccessResponse(pConnection, rootJSON.toStyledString());
}

// GET /v1/wallet/owner/retrieve_summary_info
int OwnerGetAPI::RetrieveSummaryInfo(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	WalletSummary walletSummary = walletManager.GetWalletSummary(token);

	return RestUtil::BuildSuccessResponse(pConnection, walletSummary.ToJSON().toStyledString());
}

// GET /v1/wallet/owner/retrieve_txs?refresh&id=x
int OwnerGetAPI::RetrieveTransactions(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	Json::Value rootJSON;

	const std::optional<std::string> txIdOpt = RestUtil::GetQueryParam(pConnection, "id");

	Json::Value transactionsJSON = Json::arrayValue;

	if (txIdOpt.has_value())
	{
		const uint64_t txId = std::stoull(txIdOpt.value());

		// TODO: Filter in walletManager for better performance
		const std::vector<WalletTx> transactions = walletManager.GetTransactions(token);
		for (const WalletTx& transaction : transactions)
		{
			if (transaction.GetId() == txId)
			{
				transactionsJSON.append(transaction.ToJSON());
			}
		}
	}
	else
	{
		const std::vector<WalletTx> transactions = walletManager.GetTransactions(token);
		for (const WalletTx& transaction : transactions)
		{
			transactionsJSON.append(transaction.ToJSON());
		}
	}

	rootJSON["transactions"] = transactionsJSON;
	return RestUtil::BuildSuccessResponse(pConnection, rootJSON.toStyledString());
}