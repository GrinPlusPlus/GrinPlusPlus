#pragma once

#include <Config/Config.h>
#include <Wallet/NodeClient.h>

// Forward Declarations
class WalletRestServer;
class OwnerController;
class IWalletManager;

class WalletDaemon
{
public:
	WalletDaemon(const Config& config, INodeClient& nodeClient);

	void Initialize();
	void Shutdown();

private:
	const Config& m_config;
	INodeClient& m_nodeClient;

	IWalletManager* m_pWalletManager;
	WalletRestServer* m_pWalletRestServer;
	OwnerController* m_pOwnerController;
};