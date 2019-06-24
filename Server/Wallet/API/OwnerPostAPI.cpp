#include "OwnerPostAPI.h"
#include "../../RestUtil.h"
#include "SessionTokenUtil.h"

#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletManager.h>
#include <Wallet/Exceptions/SessionTokenException.h>
#include <Wallet/Exceptions/InvalidMnemonicException.h>

int OwnerPostAPI::HandlePOST(mg_connection* pConnection, const std::string& action, IWalletManager& walletManager, INodeClient& nodeClient)
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
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return Send(pConnection, walletManager, token, requestBodyOpt.value());
	}
	else if (action == "receive_tx")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return Receive(pConnection, walletManager, token, requestBodyOpt.value());
	}
	else if (action == "finalize_tx")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return Finalize(pConnection, walletManager, token, requestBodyOpt.value());
	}
	else if (action == "post_tx")
	{
		const SessionToken token = SessionTokenUtil::GetSessionToken(*pConnection);
		return PostTx(pConnection, nodeClient, token, requestBodyOpt.value());
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "POST /v1/wallet/owner/" + action + " not Supported");
}

int OwnerPostAPI::CreateWallet(mg_connection* pConnection, IWalletManager& walletManager)
{
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

		const SecretKey grinboxKey = walletManager.GetGrinboxAddress(walletOpt.value().second);
		responseJSON["grinbox_key"] = HexUtil::ConvertToHex(grinboxKey.GetBytes().GetData());

		std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(grinboxKey);
		responseJSON["grinbox_address"] = HexUtil::ConvertToHex(pPublicKey->GetCompressedBytes().GetData());

		return RestUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Username already exists.");
	}
}

int OwnerPostAPI::Login(mg_connection* pConnection, IWalletManager& walletManager)
{
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

		const SecretKey grinboxKey = walletManager.GetGrinboxAddress(*pSessionToken);
		responseJSON["grinbox_key"] = HexUtil::ConvertToHex(grinboxKey.GetBytes().GetData());

		std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(grinboxKey);
		responseJSON["grinbox_address"] = HexUtil::ConvertToHex(pPublicKey->GetCompressedBytes().GetData());
		return RestUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
	}
	else
	{
		return RestUtil::BuildUnauthorizedResponse(pConnection, "Invalid username/password");
	}
}

int OwnerPostAPI::Logout(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	walletManager.Logout(token);

	return RestUtil::BuildSuccessResponse(pConnection, "");
}

int OwnerPostAPI::RestoreWallet(mg_connection* pConnection, IWalletManager& walletManager, const Json::Value& json)
{
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

	try
	{
		std::optional<SessionToken> tokenOpt = walletManager.RestoreFromSeed(username, password, walletWords);
		if (tokenOpt.has_value())
		{
			Json::Value responseJSON;
			responseJSON["session_token"] = std::string(tokenOpt.value().ToBase64());
			
			const SecretKey grinboxKey = walletManager.GetGrinboxAddress(tokenOpt.value());
			responseJSON["grinbox_key"] = HexUtil::ConvertToHex(grinboxKey.GetBytes().GetData());

			std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(grinboxKey);
			responseJSON["grinbox_address"] = HexUtil::ConvertToHex(pPublicKey->GetCompressedBytes().GetData());

			return RestUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
		}
		else
		{
			return RestUtil::BuildInternalErrorResponse(pConnection, "Username already exists.");
		}
	}
	catch (const InvalidMnemonicException&)
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "Mnemonic Invalid");
	}
}

int OwnerPostAPI::UpdateWallet(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	std::optional<std::string> fromGenesis = RestUtil::GetQueryParam(pConnection, "fromGenesis");
	if (walletManager.CheckForOutputs(token, fromGenesis.has_value()))
	{
		return RestUtil::BuildSuccessResponse(pConnection, "");
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "CheckForOutputs failed");
	}
}

int OwnerPostAPI::Send(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	const std::optional<uint64_t> amountOpt = JsonUtil::GetUInt64Opt(json, "amount");
	if (!amountOpt.has_value())
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

	const uint8_t numOutputs = json.get("change_outputs", Json::Value(1)).asUInt();

	std::unique_ptr<Slate> pSlate = walletManager.Send(token, amountOpt.value(), feeBaseJSON.asUInt64(), messageOpt, SelectionStrategy::FromString(selectionStrategyOpt.value()), numOutputs);
	if (pSlate != nullptr)
	{
		return RestUtil::BuildSuccessResponseJSON(pConnection, pSlate->ToJSON());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerPostAPI::Receive(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	Slate slate = Slate::FromJSON(json);

	const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message"); // TODO: Handle this

	std::unique_ptr<Slate> pReceivedSlate = walletManager.Receive(token, slate, messageOpt);
	if (pReceivedSlate != nullptr)
	{
		return RestUtil::BuildSuccessResponseJSON(pConnection, pReceivedSlate->ToJSON());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerPostAPI::Finalize(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	Slate slate = Slate::FromJSON(json);

	const bool postTx = RestUtil::HasQueryParam(pConnection, "post");

	std::unique_ptr<Slate> pFinalSlate = walletManager.Finalize(token, slate);
	if (pFinalSlate != nullptr)
	{
		walletManager.PostTransaction(token, pFinalSlate->GetTransaction());

		return RestUtil::BuildSuccessResponseJSON(pConnection, pFinalSlate->ToJSON());
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerPostAPI::PostTx(mg_connection* pConnection, INodeClient& nodeClient, const SessionToken& token, const Json::Value& json)
{
	Transaction transaction = Transaction::FromJSON(json, true);

	if (nodeClient.PostTransaction(transaction))
	{
		return RestUtil::BuildSuccessResponse(pConnection, "");
	}
	else
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerPostAPI::Repost(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	std::optional<std::string> idOpt = RestUtil::GetQueryParam(pConnection, "id");
	if (idOpt.has_value())
	{
		const uint32_t id = std::stoul(idOpt.value());
		if (walletManager.RepostByTxId(token, id))
		{
			return RestUtil::BuildSuccessResponse(pConnection, "");
		}
		else
		{
			return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
		}
	}
	else
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "id missing");
	}
}

int OwnerPostAPI::Cancel(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token)
{
	std::optional<std::string> idOpt = RestUtil::GetQueryParam(pConnection, "id");
	if (idOpt.has_value())
	{
		const uint32_t id = std::stoul(idOpt.value());
		if (walletManager.CancelByTxId(token, id))
		{
			return RestUtil::BuildSuccessResponse(pConnection, "");
		}
		else
		{
			return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
		}
	}
	else
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "id missing");
	}
}