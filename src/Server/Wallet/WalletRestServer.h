#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>

// Forward Declarations
struct WalletContext;
struct mg_context;
struct mg_connection;

class WalletRestServer
{
public:
	WalletRestServer(const Config& config, IWalletManager& walletManager, INodeClient& nodeClient);
	~WalletRestServer();

	bool Initialize();
	bool Shutdown();

private:
	static int OwnerAPIHandler(mg_connection* pConnection, void* pWalletContext);

	const Config& m_config;
	IWalletManager& m_walletManager;

	WalletContext* m_pWalletContext;
	mg_context* m_pOwnerCivetContext;
};