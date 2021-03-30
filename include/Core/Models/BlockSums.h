#pragma once

#include <Crypto/Models/Commitment.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Traits/Serializable.h>

class BlockSums : public Traits::ISerializable
{
public:
	BlockSums(Commitment&& outputSum, Commitment&& kernelSum)
		: m_outputSum(outputSum), m_kernelSum(kernelSum)
	{

	}

	BlockSums(const Commitment& outputSum, const Commitment& kernelSum)
		: m_outputSum(outputSum), m_kernelSum(kernelSum)
	{

	}

	const Commitment& GetOutputSum() const { return m_outputSum; }
	const Commitment& GetKernelSum() const { return m_kernelSum; }

	void Serialize(Serializer& serializer) const final
	{
		m_outputSum.Serialize(serializer);
		m_kernelSum.Serialize(serializer);
	}

	static BlockSums Deserialize(ByteBuffer& byteBuffer)
	{
		Commitment outputSum = Commitment::Deserialize(byteBuffer);
		Commitment kernelSum = Commitment::Deserialize(byteBuffer);

		return BlockSums(std::move(outputSum), std::move(kernelSum));
	}

private:
	Commitment m_outputSum;
	Commitment m_kernelSum;
};