#include "Dandelion.h"

#include "ConnectionManager.h"
#include "Messages/TransactionMessage.h"
#include "Messages/StemTransactionMessage.h"

#include <Common/Util/StringUtil.h>
#include <Common/Util/ThreadUtil.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Infrastructure/ThreadManager.h>
#include <Infrastructure/Logger.h>

Dandelion::Dandelion(const Config& config, ConnectionManager& connectionManager, IBlockChainServer& blockChainServer, ITransactionPool& transactionPool)
	: m_config(config), 
	m_connectionManager(connectionManager), 
	m_blockChainServer(blockChainServer), 
	m_transactionPool(transactionPool), 
	m_relayNodeId(0), 
	m_relayExpirationTime(std::chrono::system_clock::now())
{

}

void Dandelion::Start()
{
	m_terminate = true;

	if (m_dandelionThread.joinable())
	{
		m_dandelionThread.join();
	}

	m_terminate = false;

	m_dandelionThread = std::thread(Thread_Monitor, std::ref(*this));
}

void Dandelion::Stop()
{
	m_terminate = true;

	if (m_dandelionThread.joinable())
	{
		m_dandelionThread.join();
	}
}

// A process to monitor transactions in the stempool.
// With Dandelion, transactions can be broadcasted in stem or fluff phase.
// When sent in stem phase, the transaction is relayed to only 1 node: the dandelion relay.
// In order to maintain reliability a timer is started for each transaction sent in stem phase.
// This function will monitor the stempool and test if the timer is expired for each transaction. 
// In that case  the transaction will be sent in fluff phase (to multiple peers) instead of sending only to the peer relay.
void Dandelion::Thread_Monitor(Dandelion& dandelion)
{
	ThreadManagerAPI::SetCurrentThreadName("DANDELION_THREAD");
	LoggerAPI::LogDebug("Dandelion::Thread_Monitor() - BEGIN");

	const DandelionConfig& config = dandelion.m_config.GetDandelionConfig();
	while (!dandelion.m_terminate)
	{
		// This is the patience timer, we loop every n secs.
		const uint8_t patience = config.GetPatienceSeconds();
		ThreadUtil::SleepFor(std::chrono::seconds(patience), dandelion.m_terminate);

		// Step 1: find all "ToStem" entries in stempool from last run.
		// Aggregate them up to give a single (valid) aggregated tx and propagate it
		// to the next Dandelion relay along the stem.
		if (!dandelion.ProcessStemPhase())
		{
			LoggerAPI::LogError("Dandelion::Thread_Monitor() - Problem with stem phase.");
		}

		// Step 2: find all "ToFluff" entries in stempool from last run.
		// Aggregate them up to give a single (valid) aggregated tx and (re)add it
		// to our pool with stem=false (which will then broadcast it).
		if (!dandelion.ProcessFluffPhase())
		{
			LoggerAPI::LogError("Dandelion::Thread_Monitor() - Problem with fluff phase.");
		}

		// Step 3: now find all expired entries based on embargo timer.
		if (!dandelion.ProcessExpiredEntries())
		{
			LoggerAPI::LogError("Dandelion::Thread_Monitor() - Problem processing expired pool entries.");
		}
	}

	LoggerAPI::LogDebug("Dandelion::Thread_Monitor() - END");
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

	std::unique_ptr<BlockHeader> pConfirmedTipHeader = m_blockChainServer.GetTipBlockHeader(EChainType::CONFIRMED);
	std::unique_ptr<Transaction> pTransactionToStem = m_transactionPool.GetTransactionToStem(*pConfirmedTipHeader);
	if (pTransactionToStem != nullptr)
	{
		LoggerAPI::LogDebug("Dandelion::ProcessStemPhase() - Stemming transaction.");

		// Send Transaction to next Dandelion Relay.
		const StemTransactionMessage stemTransactionMessage(*pTransactionToStem);
		const bool success = m_connectionManager.SendMessageToPeer(stemTransactionMessage, m_relayNodeId);
		
		// If failed to send, fluff instead.
		if (!success)
		{
			LoggerAPI::LogWarning("Dandelion::ProcessStemPhase() - Failed to stem. Fluffing instead.");
			const bool added = m_blockChainServer.AddTransaction(*pTransactionToStem, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS;
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
	std::unique_ptr<BlockHeader> pConfirmedTipHeader = m_blockChainServer.GetTipBlockHeader(EChainType::CONFIRMED);
	std::unique_ptr<Transaction> pTransactionToFluff = m_transactionPool.GetTransactionToFluff(*pConfirmedTipHeader);
	if (pTransactionToFluff != nullptr)
	{
		LoggerAPI::LogDebug("Dandelion::ProcessFluffPhase() - Fluffing transaction.");
		const bool added = m_blockChainServer.AddTransaction(*pTransactionToFluff, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS;
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
	const std::vector<Transaction> expiredTransactions = m_transactionPool.GetExpiredTransactions();
	if (!expiredTransactions.empty())
	{
		LoggerAPI::LogInfo(StringUtil::Format("Dandelion::ProcessExpiredEntries() - %ull transactions expired. Fluffing now.", expiredTransactions.size()));
		for (const Transaction& transaction : expiredTransactions)
		{
			if (m_blockChainServer.AddTransaction(transaction, EPoolType::MEMPOOL) == EBlockChainStatus::SUCCESS)
			{
				const TransactionMessage transactionMessage(transaction);
				m_connectionManager.BroadcastMessage(transactionMessage, 0);
			}
		}
	}

	return true;
}

/*
fn process_expired_entries(
	dandelion_config: DandelionConfig,
	tx_pool: Arc<RwLock<TransactionPool>>,
) -> Result<(), PoolError> {
	let now = Utc::now().timestamp();
	let embargo_sec = dandelion_config.embargo_secs.unwrap() + thread_rng().gen_range(0, 31);
	let cutoff = now - embargo_sec as i64;

	let mut expired_entries = vec![];
	{
		let tx_pool = tx_pool.read();
		for entry in tx_pool
			.stempool
			.entries
			.iter()
			.filter(|x| x.tx_at.timestamp() < cutoff)
		{
			debug!("dand_mon: Embargo timer expired for {:?}", entry.tx.hash());
			expired_entries.push(entry.clone());
		}
	}

	if expired_entries.len() > 0 {
		debug!("dand_mon: Found {} expired txs.", expired_entries.len());

		{
			let mut tx_pool = tx_pool.write();
			let header = tx_pool.chain_head()?;

			for entry in expired_entries {
				let src = TxSource {
					debug_name: "embargo_expired".to_string(),
					identifier: "?.?.?.?".to_string(),
				};
				match tx_pool.add_to_pool(src, entry.tx, false, &header) {
					Ok(_) => debug!("dand_mon: embargo expired, fluffed tx successfully."),
					Err(e) => debug!("dand_mon: Failed to fluff expired tx - {:?}", e),
				};
			}
		}
	}
	Ok(())
}

*/