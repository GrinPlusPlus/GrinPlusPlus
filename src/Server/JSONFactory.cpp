#include "JSONFactory.h"

#include <Common/Util/TimeUtil.h>
#include <Crypto/Crypto.h>

Json::Value JSONFactory::BuildBlockJSON(const FullBlock& block)
{
	Json::Value blockNode;
	blockNode["header"] = BuildHeaderJSON(*block.GetBlockHeader());

	// Transaction Inputs
	Json::Value inputsNode;
	for (const TransactionInput& input : block.GetInputs())
	{
		inputsNode.append(BuildTransactionInputJSON(input));
	}
	blockNode["inputs"] = inputsNode;

	// Transaction Outputs
	Json::Value outputsNode;
	for (const TransactionOutput& output : block.GetOutputs())
	{
		outputsNode.append(BuildTransactionOutputJSON(output, block.GetHeight()));
	}
	blockNode["outputs"] = outputsNode;

	// Transaction Kernels
	Json::Value kernelsNode;
	for (const TransactionKernel& kernel : block.GetKernels())
	{
		kernelsNode.append(BuildTransactionKernelJSON(kernel));
	}
	blockNode["kernels"] = kernelsNode;

	return blockNode;
}

Json::Value JSONFactory::BuildCompactBlockJSON(const CompactBlock& compactBlock)
{
	Json::Value blockNode;
	blockNode["header"] = BuildHeaderJSON(*compactBlock.GetBlockHeader());


	// Short Ids
	Json::Value inputsNode;
	for (const ShortId& shortId : compactBlock.GetShortIds())
	{
		inputsNode.append(shortId.GetId().ToHex());
	}
	blockNode["inputs"] = inputsNode;

	// Transaction Outputs
	Json::Value outputsNode;
	for (const TransactionOutput& output : compactBlock.GetOutputs())
	{
		outputsNode.append(BuildTransactionOutputJSON(output, compactBlock.GetHeight()));
	}
	blockNode["out_full"] = outputsNode;

	// Transaction Kernels
	Json::Value kernelsNode;
	for (const TransactionKernel& kernel : compactBlock.GetKernels())
	{
		kernelsNode.append(BuildTransactionKernelJSON(kernel));
	}
	blockNode["kern_full"] = kernelsNode;

	return blockNode;
}

Json::Value JSONFactory::BuildHeaderJSON(const BlockHeader& header)
{
	Json::Value headerNode;
	headerNode["height"] = header.GetHeight();
	headerNode["hash"] = header.GetHash().ToHex();
	headerNode["version"] = header.GetVersion();

	headerNode["timestamp_raw"] = header.GetTimestamp();
	headerNode["timestamp_local"] = TimeUtil::FormatLocal(header.GetTimestamp());
	headerNode["timestamp"] = TimeUtil::FormatUTC(header.GetTimestamp());

	headerNode["previous"] = header.GetPreviousBlockHash().ToHex();
	headerNode["prev_root"] = header.GetPreviousRoot().ToHex();

	headerNode["kernel_root"] = header.GetKernelRoot().ToHex();
	headerNode["output_root"] = header.GetOutputRoot().ToHex();
	headerNode["range_proof_root"] = header.GetRangeProofRoot().ToHex();

	headerNode["output_mmr_size"] = header.GetOutputMMRSize();
	headerNode["kernel_mmr_size"] = header.GetKernelMMRSize();

	headerNode["total_kernel_offset"] = header.GetTotalKernelOffset().GetBytes().ToHex();
	headerNode["secondary_scaling"] = header.GetScalingDifficulty();
	headerNode["total_difficulty"] = header.GetTotalDifficulty();
	headerNode["nonce"] = header.GetNonce();

	headerNode["edge_bits"] = header.GetProofOfWork().GetEdgeBits();

	Json::Value cuckooSolution;
	for (const uint64_t proofNonce : header.GetProofOfWork().GetProofNonces())
	{
		cuckooSolution.append(proofNonce);
	}
	headerNode["cuckoo_solution"] = cuckooSolution;

	return headerNode;
}

Json::Value JSONFactory::BuildTransactionInputJSON(const TransactionInput& input)
{
	Json::Value inputNode;
	inputNode = input.GetCommitment().ToHex();
	return inputNode;
}

/*
{
	  "output_type": "Transaction",
	  "commit": "085330fdb767cbbc46a3c103cc66cf215d784e851a025c2f37ce1a57fcfa306df9",
	  "spent": true,
	  "proof": null,
	  "proof_hash": "890c52c2d69df36c9b68c26d3499ed46cc8245241226a129ef9e714c0921a2a3",
	  "block_height": null,
	  "merkle_proof": null,
	  "mmr_index": 913519
	},
*/
Json::Value JSONFactory::BuildTransactionOutputJSON(const TransactionOutput& output, const uint64_t blockHeight) // TODO: Should be OutputDTO
{
	Json::Value outputNode;

	outputNode["output_type"] = output.IsCoinbase() ? "Coinbase" : "Transaction";
	// TODO: outputNode["spent"]
	outputNode["proof"] = output.GetRangeProof().Format();

	Serializer proofSerializer;
	output.GetRangeProof().Serialize(proofSerializer);
	outputNode["proof_hash"] = Crypto::Blake2b(proofSerializer.GetBytes()).ToHex();

	outputNode["block_height"] = blockHeight;
	outputNode["mmr_index"] = 0; // TODO: This will be part of OutputDTO

	return outputNode;
}

Json::Value JSONFactory::BuildTransactionKernelJSON(const TransactionKernel& kernel)
{
	Json::Value kernelNode;

	if (kernel.GetFeatures() == EKernelFeatures::COINBASE_KERNEL)
	{
		kernelNode["features"] = "Coinbase";
	}
	else if (kernel.GetFeatures() == EKernelFeatures::HEIGHT_LOCKED)
	{
		kernelNode["features"] = "HeightLocked";
	}

	kernelNode["fee"] = kernel.GetFee();
	kernelNode["lock_height"] = kernel.GetLockHeight();
	kernelNode["excess"] = kernel.GetExcessCommitment().ToHex();
	kernelNode["excess_sig"] = kernel.GetExcessSignature().ToHex();

	return kernelNode;
}

Json::Value JSONFactory::BuildPeerJSON(const Peer& peer)
{
	Json::Value peerNode;

	peerNode["addr"] = peer.GetIPAddress().Format() + ":" + std::to_string(peer.GetPortNumber());

	Json::Value capabilitiesNode;
	capabilitiesNode["bits"] = peer.GetCapabilities().GetCapability();
	peerNode["capabilities"] = capabilitiesNode;

	peerNode["user_agent"] = peer.GetUserAgent();
	peerNode["flags"] = peer.IsBanned() ? "Banned" : "Healthy";
	peerNode["last_banned"] = Json::UInt64(peer.GetLastBanTime());
	peerNode["ban_reason"] = BanReason::Format(peer.GetBanReason());
	peerNode["last_connected"] = Json::UInt64(peer.GetLastContactTime());

	return peerNode;
}

Json::Value JSONFactory::BuildConnectedPeerJSON(const ConnectedPeer& connectedPeer)
{
	Json::Value peerNode;

	Json::Value capabilitiesNode;
	capabilitiesNode["bits"] = connectedPeer.GetPeer()->GetCapabilities().GetCapability();
	peerNode["capabilities"] = capabilitiesNode;

	PeerConstPtr peer = connectedPeer.GetPeer();
	peerNode["user_agent"] = peer->GetUserAgent();
	peerNode["version"] = peer->GetVersion();
	peerNode["addr"] = peer->GetIPAddress().Format() + ":" + std::to_string(peer->GetPortNumber());
	peerNode["direction"] = connectedPeer.GetDirection() == EDirection::OUTBOUND ? "Outbound" : "Inbound";
	peerNode["total_difficulty"] = connectedPeer.GetTotalDifficulty();
	peerNode["height"] = connectedPeer.GetHeight();

	return peerNode;
}