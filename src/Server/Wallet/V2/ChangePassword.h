#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class ChangePassword : RPCMethod
{
public:
	ChangePassword(IWalletManagerPtr pWalletManager);
	virtual ~ChangePassword() = default;

	virtual RPC::Response Handle(const RPC::Request& request) const override final;

private:
	IWalletManagerPtr m_pWalletManager;
};