#pragma once

#include "../Node/NodeContext.h"

#include <Config/Config.h>
#include <Crypto/SecretKey.h>
#include <Net/Servers/RPC/RPCServer.h>
#include <Net/Clients/RPC/RPC.h>
#include <Net/Tor/TorProcess.h>
#include <Net/Tor/TorAddress.h>
#include <atomic>
#include <thread>
#include <cassert>

class GrinJoinController
{
public:
	static std::unique_ptr<GrinJoinController> Create(
		const TorProcess::Ptr& pTorProcess,
		std::shared_ptr<NodeContext> pNodeContext,
		const std::string& privateKey
	);
	~GrinJoinController();

private:
	GrinJoinController(const TorProcess::Ptr& pTorProcess, RPCServerPtr pServer, const TorAddress& torAddress)
		: m_pTorProcess(pTorProcess), m_pServer(pServer), m_torAddress(torAddress), m_terminate(false)
	{
		assert(pServer != nullptr);
	}

	static void Thread_Process(
		GrinJoinController* pController,
		const ITransactionPoolPtr& pTransactionPool
	);

	TorProcess::Ptr m_pTorProcess;
	RPCServerPtr m_pServer;
	TorAddress m_torAddress;

	std::thread m_thread;
	std::atomic_bool m_terminate;
};