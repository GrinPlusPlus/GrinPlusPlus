#include "WalletDaemon.h"
#include "WalletRestServer.h"

#include <Wallet/WalletManager.h>

WalletDaemon::WalletDaemon(
	const Config& config,
	INodeClientPtr pNodeClient,
	IWalletManagerPtr pWalletManager,
	std::shared_ptr<WalletRestServer> pWalletRestServer)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletManager(pWalletManager),
	m_pWalletRestServer(pWalletRestServer)
{

}

std::shared_ptr<WalletDaemon> WalletDaemon::Create(const Config& config, INodeClientPtr pNodeClient)
{
	auto pWalletManager = WalletAPI::CreateWalletManager(config, pNodeClient);

	auto pWalletRestServer = WalletRestServer::Create(config, pWalletManager, pNodeClient);

	return std::make_shared<WalletDaemon>(WalletDaemon(config, pNodeClient, pWalletManager, pWalletRestServer));
}