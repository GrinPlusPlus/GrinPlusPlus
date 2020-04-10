#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCServer.h>

class OwnerController
{
public:
	~OwnerController() = default;

	static std::shared_ptr<OwnerController> Create(const Config& config, const IWalletManagerPtr& pWalletManager);

private:
	OwnerController(RPCServerPtr pServer);

	RPCServerPtr m_pServer;
};