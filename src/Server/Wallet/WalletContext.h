#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Wallet/NodeClient.h>

struct WalletContext
{
	WalletContext(const Config& config, IWalletManager* pWalletManager, INodeClient* pNodeClient)
		: m_config(config), m_pWalletManager(pWalletManager), m_pNodeClient(pNodeClient)
	{

	}

	const Config& m_config;
	IWalletManager* m_pWalletManager;
	INodeClient* m_pNodeClient;
};