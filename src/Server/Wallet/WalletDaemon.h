#pragma once

#include <Core/Config.h>
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
		std::unique_ptr<WalletRestServer>&& pWalletRestServer,
		std::unique_ptr<OwnerServer>&& pOwnerServer
	);
	~WalletDaemon();

	static std::unique_ptr<WalletDaemon> Create(
		const Config& config,
		const TorProcess::Ptr& pTorProcess,
		INodeClientPtr pNodeClient
	);

private:

	const Config& m_config;
	INodeClientPtr m_pNodeClient;
	IWalletManagerPtr m_pWalletManager;
	std::unique_ptr<WalletRestServer> m_pWalletRestServer;
	std::unique_ptr<OwnerServer> m_pOwnerServer;
};