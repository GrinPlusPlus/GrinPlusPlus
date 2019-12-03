#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class OwnerReceive : RPCMethod
{
public:
	OwnerReceive(IWalletManagerPtr pWalletManager);
	virtual ~OwnerReceive() = default;

	virtual RPC::Response Handle(const RPC::Request& request) const override final;

private:
	RPC::Response ReceiveViaFile(const RPC::Request& request, const ReceiveCriteria& criteria, const std::string& file) const;

	IWalletManagerPtr m_pWalletManager;
};