#include "WalletRestServer.h"
#include "WalletContext.h"
#include "API/OwnerGetAPI.h"
#include "API/OwnerPostAPI.h"

#include <civetweb.h>
#include <Net/Util/HTTPUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>
#include <Wallet/Exceptions/SessionTokenException.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>

WalletRestServer::WalletRestServer(const Config& config, IWalletManager& walletManager, INodeClient& nodeClient)
	: m_config(config), m_walletManager(walletManager)
{
	m_pWalletContext = new WalletContext(config, &walletManager, &nodeClient);
}

WalletRestServer::~WalletRestServer()
{
	delete m_pWalletContext;
}

bool WalletRestServer::Initialize()
{
	const uint32_t ownerPort = m_config.GetWalletConfig().GetOwnerPort();
	const char* pOwnerOptions[] = {
		"num_threads", "1",
		"listening_ports", std::to_string(ownerPort).c_str(),
		NULL
	};

	m_pOwnerCivetContext = mg_start(NULL, 0, pOwnerOptions);
	mg_set_request_handler(m_pOwnerCivetContext, "/v1/wallet/owner/", WalletRestServer::OwnerAPIHandler, m_pWalletContext);

	return true;
}

int WalletRestServer::OwnerAPIHandler(mg_connection* pConnection, void* pWalletContext)
{
	WalletContext* pContext = (WalletContext*)pWalletContext;

	try
	{
		const std::string action = HTTPUtil::GetURIParam(pConnection, "/v1/wallet/owner/");
		const HTTP::EHTTPMethod method = HTTPUtil::GetHTTPMethod(pConnection);
		if (method == HTTP::EHTTPMethod::GET)
		{
			return OwnerGetAPI::HandleGET(pConnection, action, *pContext->m_pWalletManager, *pContext->m_pNodeClient);
		}
		else if (method == HTTP::EHTTPMethod::POST)
		{
			return OwnerPostAPI(pContext->m_config).HandlePOST(pConnection, action, *pContext->m_pWalletManager, *pContext->m_pNodeClient);
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

bool WalletRestServer::Shutdown()
{
	mg_stop(m_pOwnerCivetContext);

	return true;
}