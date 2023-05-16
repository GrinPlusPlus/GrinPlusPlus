#include "WalletDaemon.h"

#include <API/Wallet/Owner/OwnerServer.h>
#include <Wallet/WalletManager.h>

WalletDaemon::WalletDaemon(
	const ServerPtr& pServer,
	const Config& config,
	IWalletManagerPtr pWalletManager,
	std::unique_ptr<OwnerServer>&& pOwnerServer)
	: m_server(pServer),
	m_config(config),
	m_pWalletManager(pWalletManager),
	m_pOwnerServer(std::move(pOwnerServer))
{

}

WalletDaemon::~WalletDaemon()
{
	LOG_INFO("Shutting down wallet daemon");
}

std::unique_ptr<WalletDaemon> WalletDaemon::Create(
	const ServerPtr& pServer,
	const Config& config,
	const TorProcess::Ptr& pTorProcess,
	const INodeClientPtr& pNodeClient)
{
	auto pWalletManager = WalletAPI::CreateWalletManager(config, pNodeClient);

	auto pOwnerServer = OwnerServer::Create(
		pServer,
		pTorProcess,
		pWalletManager
	);

	return std::make_unique<WalletDaemon>(
		pServer,
		config,
		pWalletManager,
		std::move(pOwnerServer)
	);
}