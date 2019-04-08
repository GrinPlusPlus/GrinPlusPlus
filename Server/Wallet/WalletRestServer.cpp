#include "WalletRestServer.h"
#include "WalletContext.h"
#include "../civetweb/include/civetweb.h"
#include "../RestUtil.h"
#include "API/OwnerGetAPI.h"
#include "API/OwnerPostAPI.h"

#include <Wallet/Exceptions/SessionTokenException.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>

WalletRestServer::WalletRestServer(const Config& config, IWalletManager& walletManager, INodeClient& nodeClient)
	: m_config(config), m_walletManager(walletManager)
{
	m_pWalletContext = new WalletContext(&walletManager, &nodeClient);
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
		const std::string action = RestUtil::GetURIParam(pConnection, "/v1/wallet/owner/");
		const EHTTPMethod method = RestUtil::GetHTTPMethod(pConnection);
		if (method == EHTTPMethod::GET)
		{
			return OwnerGetAPI::HandleGET(pConnection, action, *pContext->m_pWalletManager, *pContext->m_pNodeClient);
		}
		else if (method == EHTTPMethod::POST)
		{
			return OwnerPostAPI::HandlePOST(pConnection, action, *pContext->m_pWalletManager, *pContext->m_pNodeClient);
		}
	}
	catch (const SessionTokenException&)
	{
		return RestUtil::BuildUnauthorizedResponse(pConnection, "session_token is missing or invalid.");
	}
	catch (const DeserializationException&)
	{
		return RestUtil::BuildBadRequestResponse(pConnection, "Failed to deserialize one or more fields.");
	}
	catch (const InsufficientFundsException&)
	{
		return RestUtil::BuildConflictResponse(pConnection, "Insufficient funds available.");
	}
	catch (const std::exception& e)
	{
		return RestUtil::BuildInternalErrorResponse(pConnection, "Unknown error occurred." + std::string(e.what()));
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "HTTPMethod not Supported");
}

bool WalletRestServer::Shutdown()
{
	mg_stop(m_pOwnerCivetContext);

	return true;
}