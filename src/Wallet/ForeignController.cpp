#include "ForeignController.h"
#include "Keychain/KeyChain.h"

#include <Infrastructure/Logger.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorManager.h>
#include <Net/Util/HTTPUtil.h>
#include <Net/Clients/RPC/RPC.h>

ForeignController::ForeignController(const Config& config, IWalletManager& walletManager)
	: m_config(config), m_walletManager(walletManager)
{

}

ForeignController::~ForeignController()
{
	for (auto iter : m_contextsByUsername)
	{
		if (iter.second->m_torAddress.has_value())
		{
			TorManager::GetInstance(m_config.GetTorConfig()).RemoveListener(iter.second->m_torAddress.value());
		}

		mg_stop(iter.second->m_pCivetContext);
	}
}

std::optional<TorAddress> ForeignController::StartListener(const std::string& username, const SessionToken& token, const SecureVector& seed)
{
	std::unique_lock<std::mutex> lock(m_contextsMutex);

	auto iter = m_contextsByUsername.find(username);
	if (iter != m_contextsByUsername.end())
	{
		iter->second->m_numReferences++;
		return iter->second->m_torAddress;
	}

	const char* pOptions[] = {
		"num_threads", "1",
		"listening_ports", "0",
		NULL
	};

	mg_context* pForeignContext = mg_start(NULL, 0, pOptions);

	mg_server_ports ports;
	mg_get_server_ports(pForeignContext, 1, &ports);
	const uint16_t portNumber = (uint16_t)ports.port;

	std::shared_ptr<Context> pContext = std::make_shared<Context>(Context(pForeignContext, m_walletManager, portNumber, token));

	mg_set_request_handler(pForeignContext, "/v2/foreign", ForeignAPIHandler, pContext.get());
	m_contextsByUsername[username] = pContext;

	try
	{
		KeyChain keyChain = KeyChain::FromSeed(m_config, seed);
		SecretKey torKey = keyChain.DerivePrivateKey(KeyChainPath::FromString("m/0/1/0"));
		SecretKey hashedTorKey = Crypto::Blake2b(torKey.GetBytes().GetData());

		std::shared_ptr<TorAddress> pTorAddress = TorManager::GetInstance(m_config.GetTorConfig()).AddListener(hashedTorKey, portNumber);
		if (pTorAddress != nullptr)
		{
			pContext->m_torAddress = std::make_optional(*pTorAddress);
		}
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Exception thrown: %s", e.what());
	}

	return pContext->m_torAddress;
}

bool ForeignController::StopListener(const std::string& username)
{
	std::unique_lock<std::mutex> lock(m_contextsMutex);

	auto iter = m_contextsByUsername.find(username);
	if (iter == m_contextsByUsername.end())
	{
		return false;
	}

	iter->second->m_numReferences--;
	if (iter->second->m_numReferences == 0)
	{
		if (iter->second->m_torAddress.has_value())
		{
			TorManager::GetInstance(m_config.GetTorConfig()).RemoveListener(iter->second->m_torAddress.value());
		}

		mg_stop(iter->second->m_pCivetContext);

		m_contextsByUsername.erase(iter);
	}

	return true;
}

int ForeignController::ForeignAPIHandler(mg_connection* pConnection, void* pCbContext)
{
	Json::Value responseJSON;
	try
	{
		RPC::Request request = RPC::Request::Parse(pConnection);
		const std::string& method = request.GetMethod();
		if (method == "receive_tx")
		{
			const std::optional<Json::Value>& paramsOpt = request.GetParams();
			if (paramsOpt.has_value() && paramsOpt.value().isArray() && paramsOpt.value().size() == 3)
			{
				try
				{
					Context* pContext = static_cast<Context*>(pCbContext);

					Json::Value params = paramsOpt.value();
					Slate slate = Slate::FromJSON(params[0]);
					std::optional<std::string> accountName = params[1].isNull() ? std::nullopt : std::make_optional(params[1].asCString());
					std::optional<std::string> message = params[2].isNull() ? std::nullopt : std::make_optional(params[2].asCString());

					ReceiveCriteria criteria(SessionToken(pContext->m_token), std::move(slate), message, std::nullopt, std::nullopt);

					Slate receivedSlate = pContext->m_walletManager.Receive(criteria);

					Json::Value result;
					result["Ok"] = receivedSlate.ToJSON();
					responseJSON = RPC::Response::BuildResult(request.GetId(), result).ToJSON();
				}
				catch (DeserializationException&)
				{
					responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INVALID_PARAMS, "Failed to deserialize slate", std::nullopt).ToJSON();
				}
				catch (std::exception& e)
				{
					responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INTERNAL_ERROR, e.what(), std::nullopt).ToJSON();
				}
			}
			else
			{
				responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::INVALID_PARAMS, "Expected 3 parameters", std::nullopt).ToJSON();
			}
		}
		else if (method == "check_version")
		{
			Json::Value versions;
			versions["foreign_api_version"] = 2;

			Json::Value slateVersions;
			slateVersions.append("V2");
			slateVersions.append("V3");
			versions["supported_slate_versions"] = slateVersions;

			Json::Value result;
			result["Ok"] = versions;

			responseJSON = RPC::Response::BuildResult(request.GetId(), result).ToJSON();
		}
		else
		{
			responseJSON = RPC::Response::BuildError(request.GetId(), RPC::ErrorCode::METHOD_NOT_FOUND, "Method not supported", std::nullopt).ToJSON();
		}
	}
	catch (const RPCException& e)
	{
		responseJSON = RPC::Response::BuildError(e.GetId().value_or(Json::nullValue), RPC::ErrorCode::INVALID_REQUEST, e.what(), std::nullopt).ToJSON();
	}

	return HTTPUtil::BuildSuccessResponseJSON(pConnection, responseJSON);
}