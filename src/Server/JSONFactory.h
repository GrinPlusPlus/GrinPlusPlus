#pragma once

#include <json/json.h>
#include <Core/Models/BlockHeader.h>
#include <Core/Models/CompactBlock.h>
#include <Core/Models/FullBlock.h>
#include <Core/Models/Transaction.h>
#include <P2P/Peer.h>
#include <P2P/ConnectedPeer.h>

class JSONFactory
{
public:
	static Json::Value BuildBlockJSON(const FullBlock& block);
	static Json::Value BuildCompactBlockJSON(const CompactBlock& compactBlock);
	static Json::Value BuildHeaderJSON(const BlockHeader& header);

	static Json::Value BuildTransactionInputJSON(const TransactionInput& input);
	static Json::Value BuildTransactionOutputJSON(const TransactionOutput& output, const uint64_t blockHeight);
	static Json::Value BuildTransactionKernelJSON(const TransactionKernel& kernel);

	static Json::Value BuildPeerJSON(const Peer& peer);
	static Json::Value BuildConnectedPeerJSON(const ConnectedPeer& connectedPeer);
};