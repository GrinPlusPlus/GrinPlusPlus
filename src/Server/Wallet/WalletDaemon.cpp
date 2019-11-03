#include "WalletDaemon.h"
#include "WalletRestServer.h"
#include "V2/OwnerController.h"

#include <Wallet/WalletManager.h>

WalletDaemon::WalletDaemon(const Config& config, INodeClientPtr pNodeClient)
	: m_config(config), m_pNodeClient(pNodeClient)
{

}

void WalletDaemon::Initialize()
{
	m_pWalletManager = WalletAPI::CreateWalletManager(m_config, m_pNodeClient);

	m_pWalletRestServer = WalletRestServer::Create(m_config, m_pWalletManager, m_pNodeClient);
	m_pOwnerController = OwnerController::Create(m_config, m_pWalletManager);
}

void WalletDaemon::Shutdown()
{
	m_pOwnerController.reset();
	m_pWalletRestServer.reset();
}