#include <Core/Models/FullBlock.h>

FullBlock::FullBlock(BlockHeaderPtr pBlockHeader, TransactionBody&& transactionBody)
	: m_pBlockHeader(pBlockHeader), m_transactionBody(std::move(transactionBody)), m_validated(false) { }

FullBlock::FullBlock()
	: m_pBlockHeader(nullptr), m_transactionBody(), m_validated(false) { }

void FullBlock::Serialize(Serializer& serializer) const
{
	m_pBlockHeader->Serialize(serializer);
	m_transactionBody.Serialize(serializer);
}

FullBlock FullBlock::Deserialize(ByteBuffer& byteBuffer)
{
	BlockHeaderPtr pBlockHeader = std::make_shared<const BlockHeader>(BlockHeader::Deserialize(byteBuffer));
	TransactionBody transactionBody = TransactionBody::Deserialize(byteBuffer);

	return FullBlock(pBlockHeader, std::move(transactionBody));
}

Json::Value FullBlock::ToJSON() const
{
	Json::Value json;
	json["header"] = GetHeader()->ToJSON();

	// Transaction Inputs
	Json::Value inputsJSON;
	for (const TransactionInput& input : GetInputs())
	{
		inputsJSON.append(input.ToJSON());
	}
	json["inputs"] = inputsJSON;

	// Transaction Outputs
	Json::Value outputsJSON;
	for (const TransactionOutput& output : GetOutputs())
	{
		// TODO: Include MMR position?
		Json::Value outputJSON = output.ToJSON();
		outputJSON["block_height"] = GetHeight();
		outputsJSON.append(outputJSON);
	}
	json["outputs"] = outputsJSON;

	// Transaction Kernels
	Json::Value kernelsJSON;
	for (const TransactionKernel& kernel : GetKernels())
	{
		kernelsJSON.append(kernel.ToJSON());
	}
	json["kernels"] = kernelsJSON;

	return json;
}