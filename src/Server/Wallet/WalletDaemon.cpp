#include "WalletDaemon.h"
#include "V2/OwnerController.h"
#include "WalletRestServer.h"

#include <Wallet/WalletManager.h>

WalletDaemon::WalletDaemon(const Config &config, INodeClient &nodeClient) : m_config(config), m_nodeClient(nodeClient)
{
}

void WalletDaemon::Initialize()
{
    m_pWalletManager = WalletAPI::StartWalletManager(m_config, m_nodeClient);

    m_pWalletRestServer = new WalletRestServer(m_config, *m_pWalletManager, m_nodeClient);
    m_pWalletRestServer->Initialize();
    m_pOwnerController = new OwnerController(m_config);
    m_pOwnerController->StartListener(m_pWalletManager);
}

void WalletDaemon::Shutdown()
{
    m_pOwnerController->StopListener();
    m_pWalletRestServer->Shutdown();
    WalletAPI::ShutdownWalletManager(m_pWalletManager);
}