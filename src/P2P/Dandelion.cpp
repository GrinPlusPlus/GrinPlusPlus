#include "Dandelion.h"

#include "ConnectionManager.h"
#include "Messages/TransactionMessage.h"
#include "Messages/StemTransactionMessage.h"

#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>

Dandelion::Dandelion(
	const Config& config,
	ConnectionManager& connectionManager,
	IBlockChainServerPtr pBlockChainServer,
	ITransactionPoolPtr pTransactionPool,
	std::shared_ptr<const Locked<IBlockDB>> pBlockDB)
	: m_config(config), 
	m_connectionManager(connectionManager), 
	m_pBlockChainServer(pBlockChainServer),
	m_pTransactionPool(pTransactionPool),
	m_pBlockDB(pBlockDB),
	m_relayNodeId(0), 
	m_relayExpirationTime(std::chrono::system_clock::now()),
	m_terminate(false)
{

}

Dandelion::~Dandelion()
{
	m_terminate = true;
	ThreadUtil::Join(m_dandelionThread);
}

std::shared_ptr<Dandelion> Dandelion::Create(
	const Config& config,
	ConnectionManager& connectionManager,
	IBlockChainServerPtr pBlockChainServer,
	ITransactionPoolPtr pTransactionPool,
	std::shared_ptr<const Locked<IBlockDB>> pBlockDB)
{
	auto pDandelion = std::shared_ptr<Dandelion>(new Dandelion(
		config,
		connectionManager,
		pBlockChainServer,
		pTransactionPool,
		pBlockDB
	));

	pDandelion->m_dandelionThread = std::thread(Dandelion::Thread_Monitor, std::ref(*pDandelion));
	return pDandelion;
}

// A process to monitor transactions in the stempool.
// With Dandelion, transactions can be broadcasted in stem or fluff phase.
// When sent in stem phase, the transaction is relayed to only 1 node: the dandelion relay.
// In order to maintain reliability a timer is started for each transaction sent in stem phase.
// This function will monitor the stempool and test if the timer is expired for each transaction. 
// In that case  the transaction will be sent in fluff phase (to multiple peers) instead of sending only to the peer relay.
void Dandelion::Thread_Monitor(Dandelion& dandelion)
{
	ThreadManagerAPI::SetCurrentThreadName("DANDELION");
	LOG_DEBUG("BEGIN");

	const DandelionConfig& config = dandelion.m_config.GetDandelionConfig();
	while (!dandelion.m_terminate)
	{
		// This is the patience timer, we loop every n secs.
		const uint8_t patience = config.GetPatienceSeconds();
		ThreadUtil::SleepFor(std::chrono::seconds(patience), dandelion.m_terminate);

		try
		{
			// Step 1: find all "ToStem" entries in stempool from last run.
			// Aggregate them up to give a single (valid) aggregated tx and propagate it
			// to the next Dandelion relay along the stem.
			if (!dandelion.ProcessStemPhase())
			{
				LOG_ERROR("Problem with stem phase");
			}

			// Step 2: find all "ToFluff" entries in stempool from last run.
			// Aggregate them up to give a single (valid) aggregated tx and (re)add it
			// to our pool with stem=false (which will then broadcast it).
			if (!dandelion.ProcessFluffPhase())
			{
				LOG_ERROR("Problem with fluff phase");
			}

			// Step 3: now find all expired entries based on embargo timer.
			if (!dandelion.ProcessExpiredEntries())
			{
				LOG_ERROR("Problem processing expired pool entries");
			}
		}
		catch (std::exception& e)
		{
			LOG_ERROR_F("Exception thrown: %s", e.what());
		}
	}

	LOG_DEBUG("END");
}

bool Dandelion::ProcessStemPhase()
{
	if (m_relayNodeId == 0 || m_relayExpirationTime < std::chrono::system_clock::now())
	{
		const std::vector<uint64_t> mostWorkPeers = m_connectionManager.GetMostWorkPeers();
		if (mostWorkPeers.empty())
		{
			return false;
		}

		const uint16_t relaySeconds = m_config.GetDandelionConfig().GetRelaySeconds();
		m_relayExpirationTime = std::chrono::system_clock::now() + std::chrono::seconds(relaySeconds);
		const int index = RandomNumberGenerator::GenerateRandom(0, mostWorkPeers.size() - 1);
		m_relayNodeId = mostWorkPeers[index];
	}

	std::unique_ptr<Transaction> pTransactionToStem = m_pTransactionPool->GetTransactionToStem(m_pBlockDB->Read().GetShared());
	if (pTransactionToStem != nullptr)
	{
		LOG_DEBUG("Stemming transaction");

		// Send Transaction to next Dandelion Relay.
		const StemTransactionMessage stemTransactionMessage(*pTransactionToStem);
		const bool success = m_connectionManager.SendMessageToPeer(stemTransactionMessage, m_relayNodeId);
		
		// If failed to send, fluff instead.
		if (!success)
		{
			LOG_WARNING("Failed to stem, fluffing instead");
			const bool added = m_pBlockChainServer->AddTransaction(*pTransactionToStem, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS;
			if (added)
			{
				const TransactionMessage transactionMessage(*pTransactionToStem);
				m_connectionManager.BroadcastMessage(transactionMessage, 0);
			}
		}
	}

	return true;
}

bool Dandelion::ProcessFluffPhase()
{
	std::unique_ptr<Transaction> pTransactionToFluff = m_pTransactionPool->GetTransactionToFluff(m_pBlockDB->Read().GetShared());
	if (pTransactionToFluff != nullptr)
	{
		LOG_DEBUG("Fluffing transaction");
		const bool added = m_pBlockChainServer->AddTransaction(*pTransactionToFluff, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS;
		if (added)
		{
			const TransactionMessage transactionMessage(*pTransactionToFluff);
			m_connectionManager.BroadcastMessage(transactionMessage, 0);
		}
	}

	return true;
}

bool Dandelion::ProcessExpiredEntries()
{
	const std::vector<Transaction> expiredTransactions = m_pTransactionPool->GetExpiredTransactions();
	if (!expiredTransactions.empty())
	{
		LOG_INFO_F("%llu transactions expired, fluffing now", expiredTransactions.size());
		for (const Transaction& transaction : expiredTransactions)
		{
			if (m_pBlockChainServer->AddTransaction(transaction, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS)
			{
				const TransactionMessage transactionMessage(transaction);
				m_connectionManager.BroadcastMessage(transactionMessage, 0);
			}
		}
	}

	return true;
}