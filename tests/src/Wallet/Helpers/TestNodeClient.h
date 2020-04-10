#pragma once

#include <Wallet/NodeClient.h>
#include <map>

class TestNodeClient : public INodeClient
{
public:
	uint64_t GetChainHeight() const final { return 0; }
	std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>&) const final { return {}; }
	std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t, const uint64_t) const final { return {}; }
	std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t, const uint64_t) const final { return nullptr; }
	bool PostTransaction(TransactionPtr pTransaction, const EPoolType) final { return true; }
	BlockHeaderPtr GetBlockHeader(const uint64_t) const final { return nullptr; }
};