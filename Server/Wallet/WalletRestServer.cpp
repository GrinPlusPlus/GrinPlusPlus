#include "WalletRestServer.h"
#include "WalletContext.h"
#include "../civetweb/include/civetweb.h"
#include "../RestUtil.h"
#include "API/OwnerAPI.h"

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
	// Owner API
	const uint32_t ownerPort = m_config.GetWalletConfig().GetOwnerPort();
	const char* pOwnerOptions[] = {
		"num_threads", "1",
		"listening_ports", std::to_string(ownerPort).c_str(),
		NULL
	};

	m_pOwnerCivetContext = mg_start(NULL, 0, pOwnerOptions);
	mg_set_request_handler(m_pOwnerCivetContext, "/v1/wallet/owner/", OwnerAPI::Handler, m_pWalletContext);

	// Foreign API
	const uint32_t listenPort = m_config.GetWalletConfig().GetListenPort();
	const char* pForeignOptions[] = {
		"num_threads", "3",
		"listening_ports", std::to_string(listenPort).c_str(),
		NULL
	};

	m_pForeignCivetContext = mg_start(NULL, 0, pForeignOptions);
	mg_set_request_handler(m_pForeignCivetContext, "/v1/wallet/foreign/", WalletRestServer::ForeignAPIHandler, m_pWalletContext);

	return true;
}

int WalletRestServer::ForeignAPIHandler(mg_connection* pConnection, void* pWalletContext)
{
	const EHTTPMethod method = RestUtil::GetHTTPMethod(pConnection);
	if (method == EHTTPMethod::GET)
	{
		/*
			build_coinbase
			receive_tx
		*/
	}

	return RestUtil::BuildBadRequestResponse(pConnection, "Not Supported");
}

bool WalletRestServer::Shutdown()
{
	mg_stop(m_pForeignCivetContext);
	mg_stop(m_pOwnerCivetContext);

	return true;
}