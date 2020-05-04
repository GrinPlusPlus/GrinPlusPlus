#include "WalletDaemon.h"
#include "WalletRestServer.h"

#include <API/Wallet/Owner/OwnerServer.h>
#include <Wallet/WalletManager.h>

WalletDaemon::WalletDaemon(
	const Config& config,
	INodeClientPtr pNodeClient,
	IWalletManagerPtr pWalletManager,
	std::unique_ptr<WalletRestServer>&& pWalletRestServer,
	std::unique_ptr<OwnerServer>&& pOwnerServer)
	: m_config(config),
	m_pNodeClient(pNodeClient),
	m_pWalletManager(pWalletManager),
	m_pWalletRestServer(std::move(pWalletRestServer)),
	m_pOwnerServer(std::move(pOwnerServer))
{

}

WalletDaemon::~WalletDaemon()
{
	LOG_INFO("Shutting down wallet daemon");
}

std::unique_ptr<WalletDaemon> WalletDaemon::Create(
	const Config& config,
	const TorProcess::Ptr& pTorProcess,
	INodeClientPtr pNodeClient)
{
	auto pWalletManager = WalletAPI::CreateWalletManager(config, pNodeClient);

	auto pWalletRestServer = WalletRestServer::Create(
		config,
		pWalletManager,
		pNodeClient,
		pTorProcess
	);

	auto pOwnerServer = OwnerServer::Create(
		pTorProcess,
		pWalletManager
	);

	return std::make_unique<WalletDaemon>(
		config,
		pNodeClient,
		pWalletManager,
		std::move(pWalletRestServer),
		std::move(pOwnerServer)
	);
}