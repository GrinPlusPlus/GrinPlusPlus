#pragma once

#include <Config/Config.h>
#include <Wallet/WalletManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorConnection.h>
#include <Net/Servers/RPC/RPCMethod.h>
#include <optional>

class OwnerSend : RPCMethod
{
public:
	OwnerSend(const Config& config, IWalletManagerPtr pWalletManager);
	virtual ~OwnerSend() = default;

	virtual RPC::Response Handle(const RPC::Request& request) const override final;

private:
	RPC::Response SendViaTOR(const RPC::Request& request, SendCriteria& criteria, const TorAddress& torAddress) const;
	RPC::Response SendViaFile(const RPC::Request& request, const SendCriteria& criteria, const std::string& file) const;

	uint16_t CheckVersion(TorConnection& connection) const;

	const Config& m_config;
	IWalletManagerPtr m_pWalletManager;
};