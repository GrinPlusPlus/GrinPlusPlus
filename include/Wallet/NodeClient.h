#pragma once

#include <Crypto/Commitment.h>
#include <Core/Models/OutputLocation.h>
#include <Core/Models/Transaction.h>
#include <PMMR/OutputRange.h> // TODO: This belongs in Core/Models
#include <stdint.h>
#include <map>

//
// INodeClient is an interface whose implementations communicate with nodes in various ways.
// For example, an HTTPNodeClient will connect to a node using HTTP and the node's REST APIs.
//
class INodeClient
{
public:
	//
	// Returns the current confirmed chain height.
	//
	virtual uint64_t GetChainHeight() const = 0;

	//
	// Returns the location (block height and mmr index) of each requested output, if it is *unspent*.
	//
	virtual std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const = 0;

	//
	// Returns a list of outputs starting at the given insertion index.
	//
	virtual std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const = 0;

	//
	// Posts the transaction to the P2P Network.
	//
	virtual bool PostTransaction(const Transaction& transaction) = 0;
};