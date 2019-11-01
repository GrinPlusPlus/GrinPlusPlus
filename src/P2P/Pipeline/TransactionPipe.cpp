#include "TransactionPipe.h"
#include "../Messages/TransactionKernelMessage.h"
#include "../ConnectionManager.h"

#include <Common/Util/ThreadUtil.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>
#include <BlockChain/BlockChainServer.h>

TransactionPipe::TransactionPipe(const Config& config, ConnectionManager& connectionManager, IBlockChainServerPtr pBlockChainServer)
	: m_config(config), m_connectionManager(connectionManager), m_pBlockChainServer(pBlockChainServer), m_terminate(true)
{

}

void TransactionPipe::Start()
{
	if (m_terminate)
	{
		ThreadUtil::Join(m_transactionThread);

		m_terminate = false;
		m_transactionThread = std::thread(Thread_ProcessTransactions, std::ref(*this));
	}
}

void TransactionPipe::Stop()
{
	m_terminate = true;

	ThreadUtil::Join(m_transactionThread);
}

void TransactionPipe::Thread_ProcessTransactions(TransactionPipe& pipeline)
{
	ThreadManagerAPI::SetCurrentThreadName("TXN_PIPE");
	LOG_TRACE("BEGIN");

	while (!pipeline.m_terminate)
	{
		try
		{
			std::unique_ptr<TxEntry> pTxEntry = pipeline.m_transactionsToProcess.copy_front();
			if (pTxEntry != nullptr)
			{
				const bool success = (pipeline.m_pBlockChainServer->AddTransaction(pTxEntry->transaction, pTxEntry->poolType) == EBlockChainStatus::SUCCESS);
				if (success && pTxEntry->poolType == EPoolType::MEMPOOL)
				{
					// Broacast TransactionKernelMsg
					const std::vector<TransactionKernel>& kernels = pTxEntry->transaction.GetBody().GetKernels();
					for (auto& kernel : kernels)
					{
						const TransactionKernelMessage message(kernel.GetHash());
						pipeline.m_connectionManager.BroadcastMessage(message, pTxEntry->connectionId);
					}
				}

				pipeline.m_transactionsToProcess.pop_front(1);
			}
			else
			{
				ThreadUtil::SleepFor(std::chrono::milliseconds(5), pipeline.m_terminate);
			}
		}
		catch (std::exception& e)
		{
			LOG_ERROR_F("Exception caught: %s", e.what());
		}
	}

	LOG_TRACE("END");
}

bool TransactionPipe::AddTransactionToProcess(const uint64_t connectionId, const Transaction& transaction, const EPoolType poolType)
{
	std::function<bool(const TxEntry&, const TxEntry&)> comparator = [](const TxEntry& txEntry1, const TxEntry& txEntry2)
	{
		return txEntry1.transaction.GetHash() == txEntry2.transaction.GetHash();
	};

	return m_transactionsToProcess.push_back_unique(TxEntry(connectionId, transaction, poolType), comparator);
}