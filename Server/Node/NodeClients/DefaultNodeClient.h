#pragma once

#include <Wallet/NodeClient.h>
#include <BlockChain/BlockChainServer.h>
#include <Database/BlockDb.h>
#include <PMMR/TxHashSetManager.h>

class DefaultNodeClient : public INodeClient
{
public:
	DefaultNodeClient(const IBlockChainServer& blockChainServer, const IBlockDB& blockDB, const TxHashSetManager& txHashSetManager)
		: m_blockChainServer(blockChainServer), m_blockDB(blockDB), m_txHashSetManager(txHashSetManager)
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

private:
	const IBlockChainServer& m_blockChainServer;
	const IBlockDB& m_blockDB;
	const TxHashSetManager& m_txHashSetManager;
};