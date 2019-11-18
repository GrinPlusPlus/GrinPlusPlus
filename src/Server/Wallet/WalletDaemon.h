#pragma once

#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletManager.h>

// Forward Declarations
class WalletRestServer;

class WalletDaemon
{
public:
	static std::shared_ptr<WalletDaemon> Create(const Config& config, INodeClientPtr pNodeClient);
	~WalletDaemon() = default;

private:
	WalletDaemon(
		const Config& config,
		INodeClientPtr pNodeClient,
		IWalletManagerPtr pWalletManager,
		std::shared_ptr<WalletRestServer> pWalletRestServer
	);

	const Config& m_config;
	INodeClientPtr m_pNodeClient;
	IWalletManagerPtr m_pWalletManager;
	std::shared_ptr<WalletRestServer> m_pWalletRestServer;
};