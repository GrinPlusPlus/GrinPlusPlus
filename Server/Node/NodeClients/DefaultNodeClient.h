#pragma once

#include <Wallet/NodeClient.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/BlockDb.h>
#include <PMMR/TxHashSetManager.h>
#include <TxPool/TransactionPool.h>

class DefaultNodeClient : public INodeClient
{
public:
	DefaultNodeClient(const IBlockChainServer& blockChainServer, const IBlockDB& blockDB, const TxHashSetManager& txHashSetManager, ITransactionPool& transactionPool)
		: m_blockChainServer(blockChainServer), m_blockDB(blockDB), m_txHashSetManager(txHashSetManager), m_transactionPool(transactionPool)
	{

	}

	virtual uint64_t GetChainHeight() const override final
	{
		return m_blockChainServer.GetHeight(EChainType::CONFIRMED);
	}

	virtual std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const override final
	{ 
		const ITxHashSet* pTxHashSet = m_txHashSetManager.GetTxHashSet();
		if (pTxHashSet == nullptr)
		{
			return std::map<Commitment, OutputLocation>();
		}

		std::map<Commitment, OutputLocation> outputs;
		for (const Commitment& commitment : commitments)
		{
			std::optional<OutputLocation> locationOpt = m_blockDB.GetOutputPosition(commitment);
			if (locationOpt.has_value() && pTxHashSet->IsUnspent(locationOpt.value()))
			{
				outputs.insert(std::make_pair<Commitment, OutputLocation>(Commitment(commitment), OutputLocation(locationOpt.value())));
			}
		}

		return outputs;
	}

	virtual std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t startHeight, const uint64_t maxHeight) const override final
	{
		return m_blockChainServer.GetOutputsByHeight(startHeight, maxHeight);
	}

	virtual std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const override final
	{
		const ITxHashSet* pTxHashSet = m_txHashSetManager.GetTxHashSet();
		if (pTxHashSet == nullptr)
		{
			return std::unique_ptr<OutputRange>(nullptr);
		}

		return std::make_unique<OutputRange>(pTxHashSet->GetOutputsByLeafIndex(startIndex, maxNumOutputs));
	}

	virtual bool PostTransaction(const Transaction& transaction) override final
	{
		std::unique_ptr<BlockHeader> pTipHeader = m_blockChainServer.GetTipBlockHeader(EChainType::CONFIRMED);
		if (pTipHeader != nullptr)
		{
			return m_transactionPool.AddTransaction(transaction, EPoolType::STEMPOOL, *pTipHeader);
		}

		return false;
	}

private:
	const IBlockChainServer& m_blockChainServer;
	const IBlockDB& m_blockDB;
	const TxHashSetManager& m_txHashSetManager;
	ITransactionPool& m_transactionPool;
};