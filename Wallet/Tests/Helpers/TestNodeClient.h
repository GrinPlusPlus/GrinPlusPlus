#pragma once

#include <Wallet/NodeClient.h>

class TestNodeClient : public INodeClient
{
public:
	virtual uint64_t GetChainHeight() const override final { return 0; }
	virtual std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const override final { return std::map<Commitment, OutputLocation>(); }
	virtual std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const override final { return std::unique_ptr<OutputRange>(nullptr); }
	virtual bool PostTransaction(const Transaction& transaction) override final { return true; }
};