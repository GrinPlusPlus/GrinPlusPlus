#include <Core/BlockHeader.h>
#include <Crypto.h>

BlockHeader::BlockHeader
(
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

	ProofOfWork proofOfWork = ProofOfWork::Deserialize(byteBuffer);

	return BlockHeader(version, height, timestamp, std::move(previousBlockHash), std::move(previousRoot), std::move(outputRoot), std::move(rangeProofRoot), std::move(kernelRoot), std::move(totalKernelOffset), outputMMRSize, kernelMMRSize, std::move(proofOfWork));
}