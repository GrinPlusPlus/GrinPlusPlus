#include "OwnerGetAPI.h"
#include "../../RestUtil.h"
#include "SessionTokenUtil.h"

#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Wallet/SessionTokenException.h>

/*
	GET /v1/wallet/owner/node_height
	GET /v1/wallet/owner/retrieve_summary_info
	GET /v1/wallet/owner/retrieve_outputs?refresh&show_spent&tx_id=x&tx_id=y
	GET /v1/wallet/owner/retrieve_txs?refresh&id=x
	GET /v1/wallet/owner/retrieve_stored_tx?id=x
*/
int OwnerGetAPI::HandleGET(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient)
{
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

	if (txIdOpt.has_value())
	{
		// TODO: Implement this
	}
	else
	{
		Json::Value transactionsJSON = Json::arrayValue;
		const std::vector<WalletTx> transactions = walletManager.GetTransactions(token);
		for (const WalletTx& transaction : transactions)
		{
			Json::Value transactionJSON;
			transactionJSON["id"] = transaction.GetId();
			transactionJSON["type"] = WalletTxType::ToString(transaction.GetType());
			transactionJSON["amount_credited"] = transaction.GetAmountCredited();
			transactionJSON["amount_debited"] = transaction.GetAmountDebited();
			transactionJSON["creation_date_time"] = std::chrono::duration_cast<std::chrono::seconds>(transaction.GetCreationTime().time_since_epoch()).count(); // TODO: Determine format

			if (transaction.GetFee().has_value())
			{
				transactionJSON["fee"] = transaction.GetFee().value();
			}

			if (transaction.GetConfirmationTime().has_value())
			{
				transactionJSON["confirmation_date_time"] = std::chrono::duration_cast<std::chrono::seconds>(transaction.GetConfirmationTime().value().time_since_epoch()).count(); // TODO: Determine format
			}

			// TODO: Finish this
			transactionsJSON.append(transactionJSON);
		}

		rootJSON["transactions"] = transactionsJSON;
	}

	return RestUtil::BuildSuccessResponse(pConnection, rootJSON.toStyledString());
}