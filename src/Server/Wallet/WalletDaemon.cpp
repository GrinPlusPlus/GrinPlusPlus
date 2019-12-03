#include "WalletDaemon.h"
#include "WalletRestServer.h"
#include "V2/OwnerController.h"

#include <Wallet/WalletManager.h>
#include <Net/Tor/TorManager.h>

WalletDaemon::WalletDaemon(
	const Config& config,
	INodeClientPtr pNodeClient,
	IWalletManagerPtr pWalletManager,
	std::shared_ptr<WalletRestServer> pWalletRestServer,
	std::shared_ptr<OwnerController> pOwnerController)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletManager(pWalletManager),
	m_pWalletRestServer(pWalletRestServer),
	m_pOwnerController(pOwnerController)
{

}

std::shared_ptr<WalletDaemon> WalletDaemon::Create(const Config& config, INodeClientPtr pNodeClient)
{
	auto pWalletManager = WalletAPI::CreateWalletManager(config, pNodeClient);

	auto pWalletRestServer = WalletRestServer::Create(config, pWalletManager, pNodeClient);
	auto pOwnerController = OwnerController::Create(config, pWalletManager);

	TorManager::GetInstance(config.GetTorConfig());

	return std::make_shared<WalletDaemon>(WalletDaemon(config, pNodeClient, pWalletManager, pWalletRestServer, pOwnerController));
}