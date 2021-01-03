#include "Dandelion.h"

#include "ConnectionManager.h"
#include "Messages/TransactionMessage.h"
#include "Messages/StemTransactionMessage.h"

#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Crypto/CSPRNG.h>
#include <Common/Logger.h>

Dandelion::Dandelion(
	const Config& config,
	ConnectionManager& connectionManager,
	const IBlockChain::Ptr& pBlockChain,
	std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
	const ITransactionPool::Ptr& pTransactionPool,
	std::shared_ptr<const Locked<IBlockDB>> pBlockDB)
	: m_config(config), 
	m_connectionManager(connectionManager), 
	m_pBlockChain(pBlockChain),
	m_pTxHashSetManager(pTxHashSetManager),
	m_pTransactionPool(pTransactionPool),
	m_pBlockDB(pBlockDB),
	m_relayPeer(nullptr),
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
	const IBlockChain::Ptr& pBlockChain,
	std::shared_ptr<Locked<TxHashSetManager>> pTxHashSetManager,
	const ITransactionPool::Ptr& pTransactionPool,
	std::shared_ptr<const Locked<IBlockDB>> pBlockDB)
{
	auto pDandelion = std::shared_ptr<Dandelion>(new Dandelion(
		config,
		connectionManager,
		pBlockChain,
		pTxHashSetManager,
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
	LoggerAPI::SetThreadName("DANDELION");
	LOG_DEBUG("BEGIN");

	const DandelionConfig& config = dandelion.m_config.GetNodeConfig().GetDandelion();
	while (!dandelion.m_terminate)
	{
		// This is the patience timer, we loop every n secs.
		const uint8_t patience = config.GetPatienceSeconds();
		ThreadUtil::SleepFor(std::chrono::seconds(patience));

		try
		{
			// Step 1: find all "ToStem" entries in stempool from last run.
			// Aggregate them up to give a single (valid) aggregated tx and propagate it
			// to the next Dandelion relay along the stem.
			if (!dandelion.ProcessStemPhase())
			{
				LOG_TRACE("Problem with stem phase");
			}

			// Step 2: find all "ToFluff" entries in stempool from last run.
			// Aggregate them up to give a single (valid) aggregated tx and (re)add it
			// to our pool with stem=false (which will then broadcast it).
			if (!dandelion.ProcessFluffPhase())
			{
				LOG_TRACE("Problem with fluff phase");
			}

			// Step 3: now find all expired entries based on embargo timer.
			if (!dandelion.ProcessExpiredEntries())
			{
				LOG_TRACE("Problem processing expired pool entries");
			}
		}
		catch (std::exception& e)
		{
			LOG_DEBUG_F("Exception thrown: {}", e.what());
		}
	}

	LOG_DEBUG("END");
}

bool Dandelion::ProcessStemPhase()
{
	if (m_relayPeer == nullptr || m_relayExpirationTime < std::chrono::system_clock::now())
	{
		const std::vector<PeerPtr> mostWorkPeers = m_connectionManager.GetMostWorkPeers();
		if (mostWorkPeers.empty())
		{
			return false;
		}

		const uint16_t relaySeconds = m_config.GetNodeConfig().GetDandelion().GetRelaySeconds();
		m_relayExpirationTime = std::chrono::system_clock::now() + std::chrono::seconds(relaySeconds);
		const size_t index = CSPRNG::GenerateRandom(0, mostWorkPeers.size() - 1);
		m_relayPeer = mostWorkPeers[index];
	}

	TransactionPtr pTransactionToStem = nullptr;

	{
		auto locked = MultiLocker().LockShared(*m_pBlockDB, *m_pTxHashSetManager);
		auto pBlockDBReader = std::get<0>(locked);
		auto pTxHashSetManager = std::get<1>(locked);

		auto pTxHashSet = pTxHashSetManager->GetTxHashSet();
		if (pTxHashSet == nullptr)
		{
			return false;
		}

		pTransactionToStem = m_pTransactionPool->GetTransactionToStem(
			pBlockDBReader.GetShared(),
			pTxHashSet
		);
	}

	if (pTransactionToStem != nullptr)
	{
		LOG_DEBUG_F("Stemming transaction ({})", *pTransactionToStem);

		// Send Transaction to next Dandelion Relay.
		const StemTransactionMessage stemTransactionMessage(pTransactionToStem);
		const bool success = m_connectionManager.SendMessageToPeer(stemTransactionMessage, m_relayPeer);
		
		// If failed to send, fluff instead.
		if (!success)
		{
			LOG_WARNING("Failed to stem, fluffing instead");
			const bool added = m_pBlockChain->AddTransaction(pTransactionToStem, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS;
			if (added)
			{
				m_connectionManager.BroadcastMessage(TransactionMessage(pTransactionToStem), 0);
			}
		}
	}

	return true;
}

bool Dandelion::ProcessFluffPhase()
{
	TransactionPtr pTransactionToFluff = nullptr;

	{
		auto locked = MultiLocker().LockShared(*m_pBlockDB, *m_pTxHashSetManager);
		auto pBlockDBReader = std::get<0>(locked);
		auto pTxHashSetManager = std::get<1>(locked);

		auto pTxHashSet = pTxHashSetManager->GetTxHashSet();
		if (pTxHashSet == nullptr)
		{
			return false;
		}

		pTransactionToFluff = m_pTransactionPool->GetTransactionToFluff(
			pBlockDBReader.GetShared(),
			pTxHashSet
		);
	}

	if (pTransactionToFluff != nullptr)
	{
		LOG_DEBUG_F("Fluffing transaction ({})", *pTransactionToFluff);
		m_connectionManager.BroadcastMessage(TransactionMessage(pTransactionToFluff), 0);
	}

	return true;
}

bool Dandelion::ProcessExpiredEntries()
{
	const std::vector<TransactionPtr> expiredTransactions = m_pTransactionPool->GetExpiredTransactions();
	if (!expiredTransactions.empty())
	{
		LOG_INFO_F("{} transactions expired, fluffing now", expiredTransactions.size());
		for (auto& pTransaction : expiredTransactions)
		{
			if (m_pBlockChain->AddTransaction(pTransaction, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS)
			{
				m_connectionManager.BroadcastMessage(TransactionMessage(pTransaction), 0);
			}
			else
			{
				LOG_INFO_F("Failed to add {} to mempool", *pTransaction);
			}
		}
	}

	return true;
}