#include <Core/Models/TransactionBody.h>

#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Global.h>
#include <Consensus/BlockWeight.h>
#include <Consensus/HardForks.h>
#include <Crypto/Hasher.h>

TransactionBody::TransactionBody(std::vector<TransactionInput>&& inputs, std::vector<TransactionOutput>&& outputs, std::vector<TransactionKernel>&& kernels)
	: m_inputs(std::move(inputs)), m_outputs(std::move(outputs)), m_kernels(std::move(kernels))
{
	// TODO: Figure out why this is necessary. We're apparently missing a sort when creating transactions in the wallet.
	std::sort(m_inputs.begin(), m_inputs.end(), SortInputsByHash);
	std::sort(m_outputs.begin(), m_outputs.end(), SortOutputsByHash);
	std::sort(m_kernels.begin(), m_kernels.end(), SortKernelsByHash);
}

// TODO: TEMP CODE - Remove after HF
static struct
{
	bool operator()(const TransactionInput& a, const TransactionInput& b) const
	{
		Serializer a_serializer(EProtocolVersion::V2);
		a.Serialize(a_serializer);

		Serializer b_serializer(EProtocolVersion::V2);
		b.Serialize(b_serializer);
		return Hasher::Blake2b(a_serializer.GetBytes()) < Hasher::Blake2b(b_serializer.GetBytes());
	}
} SortInputsByHashV2;

void TransactionBody::Serialize(Serializer& serializer) const
{
	serializer.Append<uint64_t>(m_inputs.size());
	serializer.Append<uint64_t>(m_outputs.size());
	serializer.Append<uint64_t>(m_kernels.size());

	std::vector<TransactionInput> sorted_inputs = m_inputs;
	if (serializer.GetProtocolVersion() < EProtocolVersion::V3) {
		std::sort(sorted_inputs.begin(), sorted_inputs.end(), SortInputsByHashV2);
	}

	// Serialize Inputs
	for (const auto& input : m_inputs)
	{
		input.Serialize(serializer);
	}

	// Serialize Outputs
	for (const auto& output : m_outputs)
	{
		output.Serialize(serializer);
	}

	// Serialize Kernels
	for (const auto& kernel : m_kernels)
	{
		kernel.Serialize(serializer);
	}
}

TransactionBody TransactionBody::Deserialize(ByteBuffer& byteBuffer)
{
	const uint64_t numInputs = byteBuffer.ReadU64();
	const uint64_t numOutputs = byteBuffer.ReadU64();
	const uint64_t numKernels = byteBuffer.ReadU64();

	// Read Inputs (variable size)
	std::vector<TransactionInput> inputs;
	for (int i = 0; i < numInputs; i++)
	{
		inputs.emplace_back(TransactionInput::Deserialize(byteBuffer));
	}

	// Read Outputs (variable size)
	std::vector<TransactionOutput> outputs;
	for (int i = 0; i < numOutputs; i++)
	{
		outputs.emplace_back(TransactionOutput::Deserialize(byteBuffer));
	}

	// Read Kernels (variable size)
	std::vector<TransactionKernel> kernels;
	for (int i = 0; i < numKernels; i++)
	{
		kernels.emplace_back(TransactionKernel::Deserialize(byteBuffer));
	}

	std::sort(inputs.begin(), inputs.end(), SortInputsByHash);
	std::sort(outputs.begin(), outputs.end(), SortOutputsByHash);
	std::sort(kernels.begin(), kernels.end(), SortKernelsByHash);

	return TransactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
}

Json::Value TransactionBody::ToJSON() const
{
	Json::Value bodyNode;

	Json::Value inputsNode;
	for (const TransactionInput& input : GetInputs())
	{
		inputsNode.append(input.ToJSON());
	}
	bodyNode["inputs"] = inputsNode;

	Json::Value outputsNode;
	for (const TransactionOutput& output : GetOutputs())
	{
		outputsNode.append(output.ToJSON());
	}
	bodyNode["outputs"] = outputsNode;

	Json::Value kernelsNode;
	for (const TransactionKernel& kernel : GetKernels())
	{
		kernelsNode.append(kernel.ToJSON());
	}
	bodyNode["kernels"] = kernelsNode;

	return bodyNode;
}

TransactionBody TransactionBody::FromJSON(const Json::Value& transactionBodyJSON)
{
	// Deserialize inputs
	const Json::Value inputsJSON = JsonUtil::GetRequiredField(transactionBodyJSON, "inputs");
	std::vector<TransactionInput> inputs;
	for (size_t i = 0; i < inputsJSON.size(); i++)
	{
		inputs.emplace_back(TransactionInput::FromJSON(inputsJSON.get(Json::ArrayIndex(i), Json::nullValue)));
	}

	// Deserialize outputs
	const Json::Value outputsJSON = JsonUtil::GetRequiredField(transactionBodyJSON, "outputs");
	std::vector<TransactionOutput> outputs;
	for (size_t i = 0; i < outputsJSON.size(); i++)
	{
		outputs.emplace_back(TransactionOutput::FromJSON(outputsJSON.get(Json::ArrayIndex(i), Json::nullValue)));
	}

	// Deserialize kernels
	const Json::Value kernelsJSON = JsonUtil::GetRequiredField(transactionBodyJSON, "kernels");
	std::vector<TransactionKernel> kernels;
	for (size_t i = 0; i < kernelsJSON.size(); i++)
	{
		kernels.emplace_back(TransactionKernel::FromJSON(kernelsJSON.get(Json::ArrayIndex(i), Json::nullValue)));
	}

	std::sort(inputs.begin(), inputs.end(), SortInputsByHash);
	std::sort(outputs.begin(), outputs.end(), SortOutputsByHash);
	std::sort(kernels.begin(), kernels.end(), SortKernelsByHash);

	return TransactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
}

uint64_t TransactionBody::CalcWeight(const uint64_t block_height) const noexcept
{
	size_t num_inputs = GetInputs().size();
	size_t num_outputs = GetOutputs().size();
	size_t num_kernels = GetKernels().size();

	if (Consensus::GetHeaderVersion(Global::GetEnv(), block_height) < 5) {
		return Consensus::CalculateWeightV4(num_inputs, num_outputs, num_kernels);
	} else {
		return Consensus::CalculateWeightV5(num_inputs, num_outputs, num_kernels);
	}
}