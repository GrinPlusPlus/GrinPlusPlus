#pragma once

#include <Crypto/Models/Commitment.h>
#include <Core/Models/OutputLocation.h>
#include <Core/Models/Transaction.h>
#include <Core/Models/DTOs/BlockWithOutputs.h>
#include <Core/Models/DTOs/OutputRange.h>
#include <TxPool/PoolType.h>
#include <cstdint>
#include <memory>
#include <map>

//
// INodeClient is an interface whose implementations communicate with nodes in various ways.
// For example, an HTTPNodeClient will connect to a node using HTTP and the node's REST APIs.
//
class INodeClient
{
public:
	using UPtr = std::unique_ptr<INodeClient>;

	virtual ~INodeClient() = default;

	//
	// Returns the current confirmed chain height.
	//
	virtual uint64_t GetChainHeight() const = 0;

	//
	// Return the current confirmed chain tip header.
	//
	virtual BlockHeaderPtr GetTipHeader() const = 0;
	
	//
	// Returns the header of the confirmed block at the given height, if it exists.
	//
	virtual BlockHeaderPtr GetBlockHeader(const uint64_t height) const = 0;

	//
	// Returns the location (block height and mmr index) of each requested output, if it is *unspent*.
	//
	virtual std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const = 0;

	//
	// Returns a vector containing block ids and their outputs for the given range.
	//
	virtual std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t startHeight, const uint64_t maxHeight) const = 0;

	//
	// Returns a list of outputs starting at the given insertion index.
	//
	virtual std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const = 0;

	//
	// Posts the transaction to the P2P Network.
	//
	virtual bool PostTransaction(TransactionPtr pTransaction, const EPoolType poolType) = 0;
};

typedef std::shared_ptr<INodeClient> INodeClientPtr;
typedef std::shared_ptr<const INodeClient> INodeClientConstPtr;