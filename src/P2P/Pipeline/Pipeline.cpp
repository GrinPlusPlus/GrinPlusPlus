#include "Pipeline.h"
#include "../ConnectionManager.h"
#include "../Messages/TransactionKernelMessage.h"

#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>

Pipeline::Pipeline(const Config& config, ConnectionManager& connectionManager, IBlockChainServerPtr pBlockChainServer)
	: m_blockPipe(config, connectionManager, pBlockChainServer),
	m_transactionPipe(config, connectionManager, pBlockChainServer),
	m_txHashSetPipe(config, connectionManager, pBlockChainServer)
{

}

void Pipeline::Start()
{
	m_terminate = false;

	m_blockPipe.Start();
	m_transactionPipe.Start();
}

void Pipeline::Stop()
{
	m_terminate = true;

	m_blockPipe.Stop();
	m_transactionPipe.Stop();
	m_txHashSetPipe.Stop();
}