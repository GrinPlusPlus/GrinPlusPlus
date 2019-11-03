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

		return HTTPUtil::BuildSuccessResponse(pConnection, responseJSON.toStyledString());
	}
	else
	{
		return HTTPUtil::BuildInternalErrorResponse(pConnection, "Username already exists.");
	}
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

		const SecretKey grinboxKey = walletManager.GetGrinboxAddress(sessionToken);
		responseJSON["grinbox_key"] = HexUtil::ConvertToHex(grinboxKey.GetBytes().GetData());

		std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(grinboxKey);
		responseJSON["grinbox_address"] = HexUtil::ConvertToHex(pPublicKey->GetCompressedBytes().GetData());

		const std::optional<TorAddress> torAddressOpt = walletManager.GetTorAddress(sessionToken);
		if (torAddressOpt.has_value())
		{
			responseJSON["tor_address"] = torAddressOpt.value().ToString();
		}

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
			
			const SecretKey grinboxKey = walletManager.GetGrinboxAddress(tokenOpt.value());
			responseJSON["grinbox_key"] = HexUtil::ConvertToHex(grinboxKey.GetBytes().GetData());

			std::unique_ptr<PublicKey> pPublicKey = Crypto::CalculatePublicKey(grinboxKey);
			responseJSON["grinbox_address"] = HexUtil::ConvertToHex(pPublicKey->GetCompressedBytes().GetData());

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

int OwnerPostAPI::Send(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	std::unique_ptr<Slate> pSlate = walletManager.Send(SendCriteria::FromJSON(json));
	if (pSlate != nullptr)
	{
		return HTTPUtil::BuildSuccessResponseJSON(pConnection, pSlate->ToJSON());
	}
	else
	{
		return HTTPUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerPostAPI::Receive(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	Json::Value slateJSON = JsonUtil::GetRequiredField(json, "slate");
	Slate slate = Slate::FromJSON(slateJSON);

	const std::optional<std::string> addressOpt = JsonUtil::GetStringOpt(json, "address");
	const std::optional<std::string> messageOpt = JsonUtil::GetStringOpt(json, "message"); // TODO: Handle this

	std::unique_ptr<Slate> pReceivedSlate = walletManager.Receive(token, slate, addressOpt, messageOpt);
	if (pReceivedSlate != nullptr)
	{
		return HTTPUtil::BuildSuccessResponseJSON(pConnection, pReceivedSlate->ToJSON());
	}
	else
	{
		return HTTPUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerPostAPI::Finalize(mg_connection* pConnection, IWalletManager& walletManager, const SessionToken& token, const Json::Value& json)
{
	Slate slate = Slate::FromJSON(json);

	const bool postTx = HTTPUtil::HasQueryParam(pConnection, "post");

	std::unique_ptr<Slate> pFinalSlate = walletManager.Finalize(token, slate);
	if (pFinalSlate != nullptr)
	{
        if (postTx)
        {
            walletManager.PostTransaction(token, pFinalSlate->GetTransaction());
        }

		return HTTPUtil::BuildSuccessResponseJSON(pConnection, pFinalSlate->ToJSON());
	}
	else
	{
		return HTTPUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
}

int OwnerPostAPI::PostTx(mg_connection* pConnection, INodeClient& nodeClient, const SessionToken& token, const Json::Value& json)
{
	Transaction transaction = Transaction::FromJSON(json, true);

	if (nodeClient.PostTransaction(transaction))
	{
		return HTTPUtil::BuildSuccessResponse(pConnection, "");
	}
	else
	{
		return HTTPUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred.");
	}
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
		if (walletManager.CancelByTxId(token, id))
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