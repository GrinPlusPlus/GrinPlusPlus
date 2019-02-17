#include <Core/Models/OutputIdentifier.h>
#include <Core/Models/TransactionOutput.h>

OutputIdentifier::OutputIdentifier(const EOutputFeatures features, Commitment&& commitment)
	: m_features(features), m_commitment(std::move(commitment))
{

}

OutputIdentifier OutputIdentifier::FromOutput(const TransactionOutput& output)
{
	Commitment commitment = output.GetCommitment();

	return OutputIdentifier(output.GetFeatures(), std::move(commitment));
}

void OutputIdentifier::Serialize(Serializer& serializer) const
{
	// Serialize OutputFeatures
	serializer.Append<uint8_t>((uint8_t)m_features);
	
	// Serialize Commitment
	m_commitment.Serialize(serializer);
}

OutputIdentifier OutputIdentifier::Deserialize(ByteBuffer& byteBuffer)
{
	// Read OutputFeatures (1 byte)
	const EOutputFeatures features = (EOutputFeatures)byteBuffer.ReadU8();

	// Read Commitment (33 bytes)
	Commitment commitment = Commitment::Deserialize(byteBuffer);

	return OutputIdentifier((EOutputFeatures)features, std::move(commitment));
}