#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <civetweb/include/civetweb.h>

class OwnerController
{
public:
	OwnerController(const Config& config);

	bool StartListener(IWalletManager* pWalletManager);
	bool StopListener();

private:
	static int OwnerAPIHandler(mg_connection* pConnection, void* pContext);

	const Config& m_config;
	IWalletManager* m_pWalletManager;

	mg_context* m_pCivetContext;
};