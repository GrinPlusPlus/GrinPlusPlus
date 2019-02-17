#include "JSONFactory.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/TimeUtil.h>

Json::Value JSONFactory::BuildBlockJSON(const FullBlock& block)
{
	Json::Value blockNode;
	blockNode["header"] = BuildHeaderJSON(block.GetBlockHeader());
	blockNode["Body"] = BuildTransactionBodyJSON(block.GetTransactionBody());
	return blockNode;
}

Json::Value JSONFactory::BuildHeaderJSON(const BlockHeader& header)
{
	Json::Value headerNode;
	headerNode["Height"] = header.GetHeight();
	headerNode["Hash"] = HexUtil::ConvertToHex(header.GetHash().GetData(), false, false);
	headerNode["Version"] = header.GetVersion();

	headerNode["TimestampRaw"] = header.GetTimestamp();
	headerNode["TimestampLocal"] = TimeUtil::FormatLocal(header.GetTimestamp());
	headerNode["TimestampUTC"] = TimeUtil::FormatUTC(header.GetTimestamp());

	headerNode["PreviousHash"] = HexUtil::ConvertToHex(header.GetPreviousBlockHash().GetData(), false, false);
	headerNode["PreviousMMRRoot"] = HexUtil::ConvertToHex(header.GetPreviousRoot().GetData(), false, false);

	headerNode["KernelMMRRoot"] = HexUtil::ConvertToHex(header.GetKernelRoot().GetData(), false, false);
	headerNode["KernelMMRSize"] = header.GetKernelMMRSize();
	headerNode["TotalKernelOffset"] = HexUtil::ConvertToHex(header.GetTotalKernelOffset().GetBlindingFactorBytes().GetData(), false, false);

	headerNode["OutputMMRRoot"] = HexUtil::ConvertToHex(header.GetOutputRoot().GetData(), false, false);
	headerNode["OutputMMRSIze"] = header.GetOutputMMRSize();
	headerNode["RangeProofMMRRoot"] = HexUtil::ConvertToHex(header.GetRangeProofRoot().GetData(), false, false);

	headerNode["ScalingDifficulty"] = header.GetScalingDifficulty();
	headerNode["TotalDifficulty"] = header.GetTotalDifficulty();
	headerNode["Nonce"] = header.GetNonce();

	return headerNode;
}

Json::Value JSONFactory::BuildTransactionBodyJSON(const TransactionBody& body)
{
	Json::Value transactionBodyNode;

	// Transaction Inputs
	Json::Value inputsNode;
	for (const TransactionInput& input : body.GetInputs())
	{
		inputsNode.append(BuildTransactionInputJSON(input));
	}
	transactionBodyNode["inputs"] = inputsNode;

	// Transaction Outputs
	Json::Value outputsNode;
	for (const TransactionOutput& output : body.GetOutputs())
	{
		outputsNode.append(BuildTransactionOutputJSON(output));
	}
	transactionBodyNode["outputs"] = outputsNode;

	// Transaction Kernels
	Json::Value kernelsNode;
	for (const TransactionKernel& kernel : body.GetKernels())
	{
		kernelsNode.append(BuildTransactionKernelJSON(kernel));
	}
	transactionBodyNode["kernels"] = kernelsNode;

	return transactionBodyNode;
}

Json::Value JSONFactory::BuildTransactionInputJSON(const TransactionInput& input)
{
	Json::Value inputNode;

	// TODO: Implement

	return inputNode;
}

Json::Value JSONFactory::BuildTransactionOutputJSON(const TransactionOutput& output)
{
	Json::Value outputNode;

	// TODO: Implement

	return outputNode;
}

Json::Value JSONFactory::BuildTransactionKernelJSON(const TransactionKernel& kernel)
{
	Json::Value kernelNode;

	// TODO: Implement

	return kernelNode;
}

/*
{
	"addr": "1.0.156.224:3414",
	"capabilities": {
		"bits": 0
	},
	"user_agent": "",
	"flags": "Defunct",
	"last_banned": 0,
	"ban_reason": "None",
	"last_connected": 1550025063
}
*/
Json::Value JSONFactory::BuildPeerJSON(const Peer& peer)
{
	Json::Value peerNode;

	peerNode["addr"] = peer.GetIPAddress().Format() + ":" + std::to_string(peer.GetPortNumber());

	Json::Value capabilitiesNode;
	capabilitiesNode["bits"] = peer.GetCapabilities().GetCapability();
	peerNode["capabilities"] = capabilitiesNode;

	peerNode["user_agent"] = peer.GetUserAgent();
	peerNode["flags"] = peer.IsBanned() ? "Banned" : "Healthy";
	peerNode["last_banned"] = peer.GetLastBanTime();
	peerNode["ban_reason"] = BanReason::Format(peer.GetBanReason());
	peerNode["last_connected"] = peer.GetLastContactTime();

	return peerNode;
}

/*
{"capabilities":{"bits":15},"user_agent":"MW/Grin 1.0.0","version":1,"addr":"47.111.75.83:3414","direction":"Outbound","total_difficulty":72925553766598,"height":42840}
*/
Json::Value JSONFactory::BuildConnectedPeerJSON(const ConnectedPeer& connectedPeer)
{
	Json::Value peerNode;

	Json::Value capabilitiesNode;
	capabilitiesNode["bits"] = connectedPeer.GetPeer().GetCapabilities().GetCapability();
	peerNode["capabilities"] = capabilitiesNode;

	const Peer& peer = connectedPeer.GetPeer();
	peerNode["user_agent"] = peer.GetUserAgent();
	peerNode["version"] = peer.GetVersion();
	peerNode["addr"] = peer.GetIPAddress().Format() + ":" + std::to_string(peer.GetPortNumber());
	peerNode["direction"] = connectedPeer.GetDirection() == EDirection::OUTBOUND ? "Outbound" : "Inbound";
	peerNode["total_difficulty"] = connectedPeer.GetTotalDifficulty();
	peerNode["height"] = connectedPeer.GetHeight();

	return peerNode;
}