#pragma once

#include <Wallet/WalletManager.h>
#include <Net/Servers/RPC/RPCServer.h>
#include <API/Wallet/Owner/Handlers/CreateWalletHandler.h>
#include <API/Wallet/Owner/Handlers/SendHandler.h>
#include <API/Wallet/Owner/Handlers/ReceiveHandler.h>
#include <API/Wallet/Owner/Handlers/FinalizeHandler.h>
#include <API/Wallet/Owner/Handlers/RetryTorHandler.h>

class OwnerServer
{
public:
	using Ptr = std::shared_ptr<OwnerServer>;

	OwnerServer::OwnerServer(const RPCServerPtr& pServer) : m_pServer(pServer) { }

	static OwnerServer::Ptr OwnerServer::Create(const Config& config, const IWalletManagerPtr& pWalletManager)
	{
		RPCServerPtr pServer = RPCServer::Create(EServerType::LOCAL, std::make_optional<uint16_t>((uint16_t)3421), "/v2"); // TODO: Read port from config (Use same port as v1 owner)
		pServer->AddMethod("create_wallet", std::shared_ptr<RPCMethod>((RPCMethod*)new CreateWalletHandler(pWalletManager)));
		pServer->AddMethod("send", std::shared_ptr<RPCMethod>((RPCMethod*)new SendHandler(config, pWalletManager)));
		pServer->AddMethod("receive", std::shared_ptr<RPCMethod>((RPCMethod*)new ReceiveHandler(pWalletManager)));
		pServer->AddMethod("finalize", std::shared_ptr<RPCMethod>((RPCMethod*)new FinalizeHandler(pWalletManager)));
		pServer->AddMethod("retry_tor", std::shared_ptr<RPCMethod>((RPCMethod*)new RetryTorHandler(config, pWalletManager)));

		return std::shared_ptr<OwnerServer>(new OwnerServer(pServer));
	}

private:
	RPCServerPtr m_pServer;
};