#include "OwnerController.h"

#include "Send/OwnerSend.h"
#include "Receive/OwnerReceive.h"
#include "Finalize/OwnerFinalize.h"
#include "RetryTor.h"

OwnerController::OwnerController(RPCServerPtr pServer)
	: m_pServer(pServer)
{

}

std::shared_ptr<OwnerController> OwnerController::Create(const Config& config, IWalletManagerPtr pWalletManager)
{
	RPCServerPtr pServer = RPCServer::Create(EServerType::LOCAL, std::make_optional<uint16_t>(3421), "/v2"); // TODO: Read port from config
	pServer->AddMethod("send", std::shared_ptr<RPCMethod>((RPCMethod*)new OwnerSend(config, pWalletManager)));
	pServer->AddMethod("receive", std::shared_ptr<RPCMethod>((RPCMethod*)new OwnerReceive(pWalletManager)));
	pServer->AddMethod("finalize", std::shared_ptr<RPCMethod>((RPCMethod*)new OwnerFinalize(pWalletManager)));
	pServer->AddMethod("retry_tor", std::shared_ptr<RPCMethod>((RPCMethod*)new RetryTor(config, pWalletManager)));

	return std::shared_ptr<OwnerController>(new OwnerController(pServer));
}