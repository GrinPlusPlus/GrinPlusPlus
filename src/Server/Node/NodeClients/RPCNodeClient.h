#pragma once

#include <Wallet/NodeClient.h>
#include <Net/Clients/RPC/RPCClient.h>
#include <Common/Exceptions/UnimplementedException.h>
#include <Common/Macros.h>
#include <memory>

// TODO: Implement RetryPolicy
class RPCNodeClient : public INodeClient
{
public:
	static INodeClientPtr Create(const SocketAddress& address)
	{
		UNUSED(address);
		// TODO: Check if Grin++ Node or Grin/MW Node. If Grin++, use more efficient APIs
		return std::shared_ptr<INodeClient>(new RPCNodeClient({ std::make_shared<HTTPClient>() }));
	}

	//
	// Returns the current confirmed chain height.
	//
	uint64_t GetChainHeight() const final
	{
		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Returns the header of the confirmed block at the given height, if it exists.
	//
	BlockHeaderPtr GetBlockHeader(const uint64_t height) const final
	{
		UNUSED(height);

		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Returns the location (block height and mmr index) of each requested output, if it is *unspent*.
	//
	std::map<Commitment, OutputLocation> GetOutputsByCommitment(const std::vector<Commitment>& commitments) const final
	{
		UNUSED(commitments);

		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Returns a vector containing block ids and their outputs for the given range.
	//
	std::vector<BlockWithOutputs> GetBlockOutputs(const uint64_t startHeight, const uint64_t maxHeight) const final
	{
		UNUSED(startHeight);
		UNUSED(maxHeight);

		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Returns a list of outputs starting at the given insertion index.
	//
	std::unique_ptr<OutputRange> GetOutputsByLeafIndex(const uint64_t startIndex, const uint64_t maxNumOutputs) const final
	{
		UNUSED(startIndex);
		UNUSED(maxNumOutputs);

		throw UNIMPLEMENTED_EXCEPTION;
	}

	//
	// Posts the transaction to the P2P Network.
	//
	bool PostTransaction(TransactionPtr pTransaction, const EPoolType poolType) final
	{
		UNUSED(pTransaction);
		UNUSED(poolType);

		throw UNIMPLEMENTED_EXCEPTION;
	}

private:
	RPCNodeClient(HttpRpcClient&& client)
		: m_client(std::move(client)) { }

	HttpRpcClient m_client;
};