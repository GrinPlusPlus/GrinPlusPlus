#pragma once

#include <Wallet/WalletManager.h>
#include <Wallet/NodeClient.h>

struct WalletContext
{
	WalletContext(IWalletManager* pWalletManager, INodeClient* pNodeClient)
		: m_pWalletManager(pWalletManager), m_pNodeClient(pNodeClient)
	{

	}

	IWalletManager* m_pWalletManager;
	INodeClient* m_pNodeClient;
};