#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class OwnerFinalize : RPCMethod
{
public:
	OwnerFinalize(IWalletManagerPtr pWalletManager);
	virtual ~OwnerFinalize() = default;

	virtual RPC::Response Handle(const RPC::Request& request) const override final;

private:
	RPC::Response FinalizeViaFile(const RPC::Request& request, const FinalizeCriteria& criteria, const std::string& file) const;

	IWalletManagerPtr m_pWalletManager;
};