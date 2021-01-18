#include "TransactionPipe.h"
#include "../Messages/TransactionKernelMessage.h"
#include "../ConnectionManager.h"

#include <Common/Util/ThreadUtil.h>
#include <Common/Logger.h>
#include <BlockChain/BlockChain.h>

TransactionPipe::TransactionPipe(const Config& config, const std::shared_ptr<ConnectionManager>& pConnectionManager, const std::shared_ptr<IBlockChain>& pBlockChain)
	: m_config(config), m_pConnectionManager(pConnectionManager), m_pBlockChain(pBlockChain), m_terminate(false)
{

}

TransactionPipe::~TransactionPipe()
{
	m_terminate = true;

	ThreadUtil::Join(m_transactionThread);
}

std::shared_ptr<TransactionPipe> TransactionPipe::Create(
	const Config& config,
	const std::shared_ptr<ConnectionManager>& pConnectionManager,
	const std::shared_ptr<IBlockChain>& pBlockChain)
{
	std::shared_ptr<TransactionPipe> pTxPipe = std::shared_ptr<TransactionPipe>(new TransactionPipe(config, pConnectionManager, pBlockChain));
	pTxPipe->m_transactionThread = std::thread(Thread_ProcessTransactions, std::ref(*pTxPipe.get()));

	return pTxPipe;
}

void TransactionPipe::Thread_ProcessTransactions(TransactionPipe& pipeline)
{
	LoggerAPI::SetThreadName("TXN_PIPE");
	LOG_TRACE("BEGIN");

	while (!pipeline.m_terminate && Global::IsRunning())
	{
		try
		{
			std::unique_ptr<TxEntry> pTxEntry = pipeline.m_transactionsToProcess.copy_front();
			if (pTxEntry != nullptr)
			{
				const EBlockChainStatus status = pipeline.m_pBlockChain->AddTransaction(pTxEntry->pTransaction, pTxEntry->poolType);
				if (status == EBlockChainStatus::SUCCESS && pTxEntry->poolType == EPoolType::MEMPOOL)
				{
					// Broacast TransactionKernelMsg
					const std::vector<TransactionKernel>& kernels = pTxEntry->pTransaction->GetKernels();
					for (auto& kernel : kernels)
					{
						const TransactionKernelMessage message(kernel.GetHash());
						LOG_DEBUG_F("Broadcasting kernel {}", kernel.GetHash());
						pipeline.m_pConnectionManager->BroadcastMessage(message, pTxEntry->m_connectionId);
					}
				}
				else if (status == EBlockChainStatus::INVALID)
				{
					pTxEntry->m_peer->Ban(EBanReason::BadTransaction);
				}

				pipeline.m_transactionsToProcess.pop_front(1);
			}
			else
			{
				ThreadUtil::SleepFor(std::chrono::milliseconds(5));
			}
		}
		catch (std::exception& e)
		{
			LOG_ERROR_F("Exception caught: {}", e.what());
		}
	}

	LOG_TRACE("END");
}

bool TransactionPipe::AddTransactionToProcess(Connection& connection, const TransactionPtr& pTransaction, const EPoolType poolType)
{
	std::function<bool(const TxEntry&, const TxEntry&)> comparator = [](const TxEntry& txEntry1, const TxEntry& txEntry2)
	{
		return txEntry1.pTransaction->GetHash() == txEntry2.pTransaction->GetHash();
	};

	return m_transactionsToProcess.push_back_unique(TxEntry(connection.GetId(), connection.GetPeer(), pTransaction, poolType), comparator);
}