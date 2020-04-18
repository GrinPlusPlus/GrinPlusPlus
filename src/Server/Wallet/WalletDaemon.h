#pragma once

#include <Config/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorProcess.h>

// Forward Declarations
class WalletRestServer;
class OwnerServer;

class WalletDaemon
{
public:
	WalletDaemon(
		const Config& config,
		INodeClientPtr pNodeClient,
		IWalletManagerPtr pWalletManager,
		std::shared_ptr<WalletRestServer> pWalletRestServer,
		std::shared_ptr<OwnerServer> pOwnerServer
	);
	~WalletDaemon() = default;

	static std::shared_ptr<WalletDaemon> Create(
		const Config& config,
		const TorProcess::Ptr& pTorProcess,
		INodeClientPtr pNodeClient
	);

private:

	const Config& m_config;
	INodeClientPtr m_pNodeClient;
	IWalletManagerPtr m_pWalletManager;
	std::shared_ptr<WalletRestServer> m_pWalletRestServer;
	std::shared_ptr<OwnerServer> m_pOwnerServer;
};