#include "GrinJoinController.h"
#include "Methods/SubmitTxMethod.h"

#include <Net/Tor/TorManager.h>
#include <Net/Clients/RPC/RPC.h>
#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/ThreadManager.h>

std::shared_ptr<GrinJoinController> GrinJoinController::Create(
	const Config& config,
	std::shared_ptr<NodeContext> pNodeContext,
	const std::string& privateKey)
{
	RPCServerPtr pServer = RPCServer::Create(EServerType::PUBLIC, std::nullopt, "/v1");

	std::this_thread::sleep_for(std::chrono::seconds(10));
	auto pTorAddress = TorManager::GetInstance(config.GetTorConfig()).AddListener(privateKey, pServer->GetPortNumber());
	if (pTorAddress == nullptr)
	{
		throw TOR_EXCEPTION("Failed to start TOR listener");
	}

	pServer->AddMethod("submit_tx", std::make_shared<SubmitTxMethod>(pNodeContext));

	auto pController = std::shared_ptr<GrinJoinController>(new GrinJoinController(config, pServer, *pTorAddress));

	pController->m_thread = std::thread(Thread_Process, pController.get(), pNodeContext->m_pTransactionPool);

	return pController;
}

GrinJoinController::~GrinJoinController()
{
	m_terminate = true;
	ThreadUtil::Join(m_thread);

	TorManager::GetInstance(m_config.GetTorConfig()).RemoveListener(m_torAddress);
}

void GrinJoinController::Thread_Process(GrinJoinController* pController, ITransactionPoolPtr pTransactionPool)
{
	ThreadManagerAPI::SetCurrentThreadName("GrinJoin");

	while (!pController->m_terminate)
	{
		try
		{
			ThreadUtil::SleepFor(std::chrono::minutes(2), pController->m_terminate);

			LOG_INFO("Fluffing JoinPool");
			pTransactionPool->FluffJoinPool();
		}
		catch (std::exception& e)
		{
			LOG_ERROR_F("Exception thrown: {}", e.what());
		}
	}
}