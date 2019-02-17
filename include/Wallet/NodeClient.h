#pragma once

#include <Crypto/Commitment.h>
#include <stdint.h>

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
	// Returns the number of blocks that confirm the output.
	// Ex: If an output was added in block 50, and the chain is currently on block 53, this will return 4 (confirmed on blocks 50, 51, 52, and 53).
	//
	virtual uint64_t GetNumConfirmations(const Commitment& outputCommitment) const = 0;
};