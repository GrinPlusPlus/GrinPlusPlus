#include <Core/Models/CompactBlock.h>

CompactBlock::CompactBlock(
	BlockHeaderPtr pBlockHeader,
	const uint64_t nonce,
	std::vector<TransactionOutput>&& fullOutputs,
	std::vector<TransactionKernel>&& fullKernels,
	std::vector<ShortId>&& shortIds)
	: m_pBlockHeader(pBlockHeader),
	m_nonce(nonce),
	m_outputs(std::move(fullOutputs)),
	m_kernels(std::move(fullKernels)),
	m_shortIds(std::move(shortIds))
{

}

void CompactBlock::Serialize(Serializer& serializer) const
{
	m_pBlockHeader->Serialize(serializer);
	serializer.Append<uint64_t>(m_nonce);
	
	serializer.Append<uint64_t>(m_outputs.size());
	serializer.Append<uint64_t>(m_kernels.size());
	serializer.Append<uint64_t>(m_shortIds.size());

	for (const TransactionOutput& output : m_outputs)
	{
		output.Serialize(serializer);
	}

	for (const TransactionKernel& kernel : m_kernels)
	{
		kernel.Serialize(serializer);
	}

	for (const ShortId& shortId : m_shortIds)
	{
		shortId.Serialize(serializer);
	}
}

CompactBlock CompactBlock::Deserialize(ByteBuffer& byteBuffer)
{
	BlockHeaderPtr pBlockHeader = std::make_shared<const BlockHeader>(BlockHeader::Deserialize(byteBuffer));
	const uint64_t nonce = byteBuffer.ReadU64();

	const uint64_t numOutputs = byteBuffer.ReadU64();
	const uint64_t numKernels = byteBuffer.ReadU64();
	const uint64_t numShortIds = byteBuffer.ReadU64();

	std::vector<TransactionOutput> outputs;
	for (int i = 0; i < numOutputs; i++)
	{
		outputs.emplace_back(TransactionOutput::Deserialize(byteBuffer));
	}

	std::vector<TransactionKernel> kernels;
	for (int i = 0; i < numKernels; i++)
	{
		kernels.emplace_back(TransactionKernel::Deserialize(byteBuffer));
	}

	std::vector<ShortId> shortIds;
	for (int i = 0; i < numShortIds; i++)
	{
		shortIds.emplace_back(ShortId::Deserialize(byteBuffer));
	}

	return CompactBlock(pBlockHeader, nonce, std::move(outputs), std::move(kernels), std::move(shortIds));
}

Json::Value CompactBlock::ToJSON() const
{
	Json::Value json;
	json["header"] = GetHeader()->ToJSON();

	// Short Ids
	Json::Value inputsNode;
	for (const ShortId& shortId : GetShortIds())
	{
		json.append(shortId.GetId().ToHex());
	}
	json["inputs"] = inputsNode;

	// Transaction Outputs
	Json::Value outputsNode;
	for (const TransactionOutput& output : GetOutputs())
	{
		// TODO: Include MMR position?
		Json::Value outputJSON = output.ToJSON();
		outputJSON["block_height"] = GetHeight();
		outputsNode.append(outputJSON);
	}
	json["outputs"] = outputsNode;

	// Transaction Kernels
	Json::Value kernelsNode;
	for (const TransactionKernel& kernel : GetKernels())
	{
		kernelsNode.append(kernel.ToJSON());
	}
	json["kernels"] = kernelsNode;

	return json;
}