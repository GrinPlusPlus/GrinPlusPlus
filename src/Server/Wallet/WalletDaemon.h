#pragma once

#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletManager.h>

// Forward Declarations
class WalletRestServer;
class OwnerController;

class WalletDaemon
{
public:
	WalletDaemon(const Config& config, INodeClientPtr pNodeClient);

	void Initialize();
	void Shutdown();

private:
	const Config& m_config;
	INodeClientPtr m_pNodeClient;
	IWalletManagerPtr m_pWalletManager;
	std::shared_ptr<WalletRestServer> m_pWalletRestServer;
	std::shared_ptr<OwnerController> m_pOwnerController;
};