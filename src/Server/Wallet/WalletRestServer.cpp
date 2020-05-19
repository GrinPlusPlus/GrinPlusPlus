#include "WalletRestServer.h"
#include "WalletContext.h"
#include "API/OwnerGetAPI.h"
#include "API/OwnerPostAPI.h"

#include <civetweb.h>
#include <Net/Util/HTTPUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>
#include <Wallet/Exceptions/SessionTokenException.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>

WalletRestServer::WalletRestServer(
	const Config& config,
	const IWalletManagerPtr& pWalletManager,
	std::unique_ptr<WalletContext>&& pWalletContext,
	mg_context* pOwnerCivetContext)
	: m_config(config),
	m_pWalletManager(pWalletManager),
	m_pWalletContext(std::move(pWalletContext)),
	m_pOwnerCivetContext(pOwnerCivetContext)
{

}

WalletRestServer::~WalletRestServer()
{
	LOG_INFO("Shutting down wallet rest server");
	mg_stop(m_pOwnerCivetContext);
}

std::unique_ptr<WalletRestServer> WalletRestServer::Create(
	const Config& config,
	const IWalletManagerPtr& pWalletManager,
	const std::shared_ptr<INodeClient>& pNodeClient,
	const TorProcess::Ptr& pTorProcess)
{
	const uint32_t ownerPort = config.GetWalletConfig().GetOwnerPort();
	const std::string listeningPorts = StringUtil::Format("127.0.0.1:{}", ownerPort);
	const char* pOwnerOptions[] = {
		"num_threads", "5",
		"listening_ports", listeningPorts.c_str(),
		NULL
	};

	std::unique_ptr<WalletContext> pWalletContext = std::make_unique<WalletContext>(
		config, pWalletManager, pNodeClient, pTorProcess
	);

	mg_context* pOwnerCivetContext = mg_start(NULL, 0, pOwnerOptions);
	mg_set_request_handler(pOwnerCivetContext, "/v1/wallet/owner/", WalletRestServer::OwnerAPIHandler, pWalletContext.get());

	return std::make_unique<WalletRestServer>(
		config,
		pWalletManager,
		std::move(pWalletContext),
		pOwnerCivetContext
	);
}

int WalletRestServer::OwnerAPIHandler(mg_connection* pConnection, void* pWalletContext)
{
	try
	{
		WalletContext* pContext = (WalletContext*)pWalletContext;

		const std::string action = HTTPUtil::GetURIParam(pConnection, "/v1/wallet/owner/");
		const HTTP::EHTTPMethod method = HTTPUtil::GetHTTPMethod(pConnection);
		if (method == HTTP::EHTTPMethod::GET)
		{
			return OwnerGetAPI::HandleGET(pConnection, action, *pContext->m_pWalletManager, *pContext->m_pNodeClient);
		}
		else if (method == HTTP::EHTTPMethod::POST)
		{
			OwnerPostAPI postAPI(pContext->m_config, pContext->m_pTorProcess);
			return postAPI.HandlePOST(pConnection, action, *pContext->m_pWalletManager);
		}
	}
	catch (const SessionTokenException&)
	{
		return HTTPUtil::BuildUnauthorizedResponse(pConnection, "session_token is missing or invalid.");
	}
	catch (const DeserializationException&)
	{
		return HTTPUtil::BuildBadRequestResponse(pConnection, "Failed to deserialize one or more fields.");
	}
	catch (const InsufficientFundsException&)
	{
		return HTTPUtil::BuildConflictResponse(pConnection, "Insufficient funds available.");
	}
	catch (const WalletStoreException& e)
	{
		return HTTPUtil::BuildInternalErrorResponse(pConnection, std::string(e.what()));
	}
	catch (const std::exception& e)
	{
		return HTTPUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred." + std::string(e.what()));
	}

	return HTTPUtil::BuildBadRequestResponse(pConnection, "HTTPMethod not Supported");
}