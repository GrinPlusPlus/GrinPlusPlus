#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Wallet/NodeClient.h>

struct WalletContext
{
	WalletContext(
		const Config& config,
		IWalletManagerPtr pWalletManager,
		INodeClientPtr pNodeClient,
		const TorProcess::Ptr& pTorProcess)
		: m_config(config),
		m_pWalletManager(pWalletManager),
		m_pNodeClient(pNodeClient),
		m_pTorProcess(pTorProcess)
	{

	}

	const Config& m_config;
	IWalletManagerPtr m_pWalletManager;
	INodeClientPtr m_pNodeClient;
	TorProcess::Ptr m_pTorProcess;
};