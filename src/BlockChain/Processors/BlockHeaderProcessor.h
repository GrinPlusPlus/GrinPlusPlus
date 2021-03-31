#pragma once

#include "../ChainState.h"

#include <BlockChain/BlockChainStatus.h>
#include <Core/Models/BlockHeader.h>

class BlockHeaderProcessor
{
public:
	BlockHeaderProcessor(const std::shared_ptr<Locked<ChainState>>& pChainState)
		: m_pChainState(pChainState) { }

	//
	// Validates and adds a single header to the candidate chain.
	//
	// Throws BadDataException if the header is invalid.
	// Throws BlockChainException if any other errors occur.
	//
	EBlockChainStatus ProcessSingleHeader(const BlockHeaderPtr& pHeader);

	//
	// Validates and adds multiple headers to the sync chain.
	// The headers are also added to the candidate chain if total difficulty increases.
	//
	// Throws BadDataException if any of the headers are invalid.
	// Throws BlockChainException if any other errors occur.
	//
	EBlockChainStatus ProcessSyncHeaders(const std::vector<BlockHeaderPtr>& headers);

private:
	EBlockChainStatus ProcessOrphan(
		Writer<ChainState> pLockedState,
		BlockHeaderPtr pHeader
	);

	EBlockChainStatus ProcessChunkedSyncHeaders(
		const std::vector<BlockHeaderPtr>& headers
	);

	void PrepareSyncChain(
		Writer<ChainState> pLockedState,
		const std::vector<BlockHeaderPtr>& headers
	);

	void RewindMMR(
		Writer<ChainState> pLockedState,
		const std::vector<BlockHeaderPtr>& headers
	);

	void ValidateHeaders(
		Writer<ChainState> pLockedState,
		const std::vector<BlockHeaderPtr>& headers
	);

	std::shared_ptr<Locked<ChainState>> m_pChainState;
};