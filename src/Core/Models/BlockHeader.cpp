#include <Core/Models/BlockHeader.h>
#include <Common/Util/TimeUtil.h>
#include <Core/Util/JsonUtil.h>

BlockHeader::BlockHeader(
	const uint16_t version,
	const uint64_t height,
	const int64_t timestamp,
	CBigInteger<32>&& previousBlockHash,
	CBigInteger<32>&& previousRoot,
	CBigInteger<32>&& outputRoot,
	CBigInteger<32>&& rangeProofRoot,
	CBigInteger<32>&& kernelRoot,
	BlindingFactor&& totalKernelOffset,
	const uint64_t outputMMRSize,
	const uint64_t kernelMMRSize,
	const uint64_t totalDifficulty,
	const uint32_t scalingDifficulty,
	const uint64_t nonce,
	ProofOfWork&& proofOfWork
)
	: m_version(version),
	m_height(height),
	m_timestamp(timestamp),
	m_previousBlockHash(std::move(previousBlockHash)),
	m_previousRoot(std::move(previousRoot)),
	m_outputRoot(std::move(outputRoot)),
	m_rangeProofRoot(std::move(rangeProofRoot)),
	m_kernelRoot(std::move(kernelRoot)),
	m_totalKernelOffset(std::move(totalKernelOffset)),
	m_outputMMRSize(outputMMRSize),
	m_kernelMMRSize(kernelMMRSize),
	m_totalDifficulty(totalDifficulty),
	m_scalingDifficulty(scalingDifficulty),
	m_nonce(nonce),
	m_proofOfWork(std::move(proofOfWork))
{

}

void BlockHeader::Serialize(Serializer& serializer) const
{
	serializer.Append<uint16_t>(m_version);
	serializer.Append<uint64_t>(m_height);
	serializer.Append<int64_t>(m_timestamp);
	serializer.AppendBigInteger<32>(m_previousBlockHash);
	serializer.AppendBigInteger<32>(m_previousRoot);
	serializer.AppendBigInteger<32>(m_outputRoot);
	serializer.AppendBigInteger<32>(m_rangeProofRoot);
	serializer.AppendBigInteger<32>(m_kernelRoot);
	m_totalKernelOffset.Serialize(serializer);
	serializer.Append<uint64_t>(m_outputMMRSize);
	serializer.Append<uint64_t>(m_kernelMMRSize);
	serializer.Append<uint64_t>(m_totalDifficulty);
	serializer.Append<uint32_t>(m_scalingDifficulty);
	serializer.Append<uint64_t>(m_nonce);
	m_proofOfWork.Serialize(serializer);
}

BlockHeader BlockHeader::Deserialize(ByteBuffer& byteBuffer)
{
	const uint16_t version = byteBuffer.ReadU16();
	const uint64_t height = byteBuffer.ReadU64();
	const int64_t timestamp = byteBuffer.Read64();

	CBigInteger<32> previousBlockHash = byteBuffer.ReadBigInteger<32>();
	CBigInteger<32> previousRoot = byteBuffer.ReadBigInteger<32>();
	CBigInteger<32> outputRoot = byteBuffer.ReadBigInteger<32>();
	CBigInteger<32> rangeProofRoot = byteBuffer.ReadBigInteger<32>();
	CBigInteger<32> kernelRoot = byteBuffer.ReadBigInteger<32>();
	BlindingFactor totalKernelOffset = BlindingFactor::Deserialize(byteBuffer);

	const uint64_t outputMMRSize = byteBuffer.ReadU64();
	const uint64_t kernelMMRSize = byteBuffer.ReadU64();

	const uint64_t totalDifficulty = byteBuffer.ReadU64();
	const uint32_t scalingDifficulty = byteBuffer.ReadU32();
	const uint64_t nonce = byteBuffer.ReadU64();

	ProofOfWork proofOfWork = ProofOfWork::Deserialize(byteBuffer);

	return BlockHeader(
		version, 
		height, 
		timestamp,
		std::move(previousBlockHash), 
		std::move(previousRoot), 
		std::move(outputRoot), 
		std::move(rangeProofRoot), 
		std::move(kernelRoot), 
		std::move(totalKernelOffset), 
		outputMMRSize, 
		kernelMMRSize,
		totalDifficulty,
		scalingDifficulty,
		nonce,
		std::move(proofOfWork)
	);
}

Json::Value BlockHeader::ToJSON() const
{
	Json::Value headerJSON;
	headerJSON["height"] = GetHeight();
	headerJSON["hash"] = GetHash().ToHex();
	headerJSON["version"] = GetVersion();

	headerJSON["timestamp_raw"] = GetTimestamp();
	headerJSON["timestamp_local"] = TimeUtil::FormatLocal(GetTimestamp());
	headerJSON["timestamp"] = TimeUtil::FormatUTC(GetTimestamp());

	headerJSON["previous"] = GetPreviousHash().ToHex();
	headerJSON["prev_root"] = GetPreviousRoot().ToHex();

	headerJSON["kernel_root"] = GetKernelRoot().ToHex();
	headerJSON["output_root"] = GetOutputRoot().ToHex();
	headerJSON["range_proof_root"] = GetRangeProofRoot().ToHex();

	headerJSON["output_mmr_size"] = GetOutputMMRSize();
	headerJSON["kernel_mmr_size"] = GetKernelMMRSize();

	headerJSON["total_kernel_offset"] = GetTotalKernelOffset().GetBytes().ToHex();
	headerJSON["secondary_scaling"] = GetScalingDifficulty();
	headerJSON["total_difficulty"] = GetTotalDifficulty();
	headerJSON["nonce"] = GetNonce();

	headerJSON["edge_bits"] = GetProofOfWork().GetEdgeBits();

	Json::Value cuckooSolution;
	for (const uint64_t proofNonce : GetProofOfWork().GetProofNonces())
	{
		cuckooSolution.append(proofNonce);
	}
	headerJSON["cuckoo_solution"] = cuckooSolution;
	return headerJSON;
}

std::shared_ptr<BlockHeader> BlockHeader::FromJSON(const Json::Value& json)
{
	Hash hash = JsonUtil::GetHash(json, "hash");
	uint64_t height = JsonUtil::GetRequiredUInt64(json, "height");
	uint16_t version = JsonUtil::GetRequiredUInt16(json, "version");
	BlindingFactor totalKernelOffset = JsonUtil::GetBlindingFactor(json, "total_kernel_offset");
	uint32_t secondaryScaling = JsonUtil::GetRequiredUInt32(json, "secondary_scaling");
	uint64_t totalDifficulty = JsonUtil::GetRequiredUInt64(json, "total_difficulty");
	uint64_t nonce = JsonUtil::GetRequiredUInt64(json, "nonce");
	time_t timestamp = TimeUtil::FromString(JsonUtil::GetRequiredString(json, "timestamp"));

	Hash kernelRoot = JsonUtil::GetHash(json, "kernel_root");
	uint64_t kernelSize = JsonUtil::GetRequiredUInt64(json, "kernel_mmr_size");
	Hash outputRoot = JsonUtil::GetHash(json, "output_root");
	uint64_t outputSize = JsonUtil::GetRequiredUInt64(json, "output_mmr_size");
	Hash rangeProofRoot = JsonUtil::GetHash(json, "range_proof_root");
	Hash prevRoot = JsonUtil::GetHash(json, "prev_root");
	Hash prevHash = JsonUtil::GetHash(json, "previous");

	// ProofOfWork
	uint8_t edgeBits = JsonUtil::GetRequiredUInt8(json, "edge_bits");
	std::vector<uint64_t> proofNonces;
	for (auto cuckoo_nonce : JsonUtil::GetRequiredField(json, "cuckoo_solution"))
	{
		proofNonces.push_back(cuckoo_nonce.asUInt64());
	}

	return std::make_shared<BlockHeader>(
		version,
		height,
		timestamp,
		std::move(prevHash),
		std::move(prevRoot),
		std::move(outputRoot),
		std::move(rangeProofRoot),
		std::move(kernelRoot),
		std::move(totalKernelOffset),
		outputSize,
		kernelSize,
		totalDifficulty,
		secondaryScaling,
		nonce,
		ProofOfWork{edgeBits, std::move(proofNonces)}
	);
}

std::vector<unsigned char> BlockHeader::GetPreProofOfWork() const
{
	Serializer serializer;
	serializer.Append<uint16_t>(m_version);
	serializer.Append<uint64_t>(m_height);
	serializer.Append<int64_t>(m_timestamp);
	serializer.AppendBigInteger<32>(m_previousBlockHash);
	serializer.AppendBigInteger<32>(m_previousRoot);
	serializer.AppendBigInteger<32>(m_outputRoot);
	serializer.AppendBigInteger<32>(m_rangeProofRoot);
	serializer.AppendBigInteger<32>(m_kernelRoot);
	m_totalKernelOffset.Serialize(serializer);
	serializer.Append<uint64_t>(m_outputMMRSize);
	serializer.Append<uint64_t>(m_kernelMMRSize);
	serializer.Append<uint64_t>(m_totalDifficulty);
	serializer.Append<uint32_t>(m_scalingDifficulty);
	serializer.Append<uint64_t>(m_nonce);

	return serializer.GetBytes();
}