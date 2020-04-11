#include "WalletDaemon.h"
#include "WalletRestServer.h"

#include <API/Wallet/Owner/OwnerServer.h>
#include <Wallet/WalletManager.h>
#include <Net/Tor/TorManager.h>

WalletDaemon::WalletDaemon(
	const Config& config,
	INodeClientPtr pNodeClient,
	IWalletManagerPtr pWalletManager,
	std::shared_ptr<WalletRestServer> pWalletRestServer,
	std::shared_ptr<OwnerServer> pOwnerServer)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletManager(pWalletManager),
	m_pWalletRestServer(pWalletRestServer),
	m_pOwnerServer(pOwnerServer)
{

}

std::shared_ptr<WalletDaemon> WalletDaemon::Create(const Config& config, INodeClientPtr pNodeClient)
{
	auto pWalletManager = WalletAPI::CreateWalletManager(config, pNodeClient);

	auto pWalletRestServer = WalletRestServer::Create(config, pWalletManager, pNodeClient);
	auto pOwnerServer = OwnerServer::Create(config, pWalletManager);

	TorManager::GetInstance(config.GetTorConfig());

	return std::make_shared<WalletDaemon>(config, pNodeClient, pWalletManager, pWalletRestServer, pOwnerServer);
}