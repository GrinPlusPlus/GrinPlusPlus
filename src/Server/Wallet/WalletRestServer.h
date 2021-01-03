#pragma once

#include <Core/Config.h>
#include <Wallet/WalletManager.h>

// Forward Declarations
struct WalletContext;
struct mg_context;
struct mg_connection;

class WalletRestServer
{
public:
	WalletRestServer(
		const Config& config,
		const IWalletManagerPtr& pWalletManager,
		std::unique_ptr<WalletContext>&& pWalletContext,
		mg_context* pOwnerCivetContext
	);
	~WalletRestServer();

	static std::unique_ptr<WalletRestServer> Create(
		const Config& config,
		const IWalletManagerPtr& pWalletManager,
		const std::shared_ptr<INodeClient>& pNodeClient,
		const TorProcess::Ptr& pTorProcess
	);

private:
	static int OwnerAPIHandler(mg_connection* pConnection, void* pWalletContext);

	const Config& m_config;
	IWalletManagerPtr m_pWalletManager;
	std::shared_ptr<WalletContext> m_pWalletContext;
	mg_context* m_pOwnerCivetContext;
};