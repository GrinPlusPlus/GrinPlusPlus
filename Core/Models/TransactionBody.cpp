#include <Core/Models/TransactionBody.h>

#include <Serialization/Serializer.h>

TransactionBody::TransactionBody(std::vector<TransactionInput>&& inputs, std::vector<TransactionOutput>&& outputs, std::vector<TransactionKernel>&& kernels)
	: m_inputs(std::move(inputs)), m_outputs(std::move(outputs)), m_kernels(std::move(kernels))
{

}

void TransactionBody::Serialize(Serializer& serializer) const
{
	serializer.Append<uint64_t>(m_inputs.size());
	serializer.Append<uint64_t>(m_outputs.size());
	serializer.Append<uint64_t>(m_kernels.size());

	// Serialize Inputs
	for (auto input : m_inputs)
	{
		input.Serialize(serializer);
	}

	// Serialize Outputs
	for (auto output : m_outputs)
	{
		output.Serialize(serializer);
	}

	// Serialize Kernels
	for (auto kernel : m_kernels)
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

	return TransactionBody(std::move(inputs), std::move(outputs), std::move(kernels));
}