#pragma once

#include <Core/Config.h>
#include <Wallet/NodeClient.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorProcess.h>

// Forward Declarations
class OwnerServer;

class WalletDaemon
{
public:
	WalletDaemon(
		const Config& config,
		IWalletManagerPtr pWalletManager,
		std::unique_ptr<OwnerServer>&& pOwnerServer
	);
	~WalletDaemon();

	static std::unique_ptr<WalletDaemon> Create(
		const Config& config,
		const TorProcess::Ptr& pTorProcess,
		const INodeClientPtr& pNodeClient
	);

private:

	const Config& m_config;
	IWalletManagerPtr m_pWalletManager;
	std::unique_ptr<OwnerServer> m_pOwnerServer;
};