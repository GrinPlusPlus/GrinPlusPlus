#include <Core/TransactionOutput.h>
#include <Crypto.h>

TransactionOutput::TransactionOutput(const EOutputFeatures features, Commitment&& commitment, RangeProof&& rangeProof)
	: m_features(features), m_commitment(std::move(commitment)), m_rangeProof(std::move(rangeProof))
{

}

void TransactionOutput::Serialize(Serializer& serializer) const
{
	// Serialize OutputFeatures
	serializer.Append<uint8_t>((uint8_t)m_features);
	
	// Serialize Commitment
	m_commitment.Serialize(serializer);

	// Serialize RangeProof
	m_rangeProof.Serialize(serializer);
}

TransactionOutput TransactionOutput::Deserialize(ByteBuffer& byteBuffer)
{
	// Read OutputFeatures (1 byte)
	const EOutputFeatures features = (EOutputFeatures)byteBuffer.ReadU8();

	// Read Commitment (33 bytes)
	Commitment commitment = Commitment::Deserialize(byteBuffer);

	// Read RangeProof (variable size)
	RangeProof rangeProof = RangeProof::Deserialize(byteBuffer);

	return TransactionOutput((EOutputFeatures)features, std::move(commitment), std::move(rangeProof));
}

const CBigInteger<32>& TransactionOutput::Hash() const
{
	if (m_hash == CBigInteger<32>())
	{
		Serializer serializer;
		
		// Serialize OutputFeatures
		serializer.Append<uint8_t>((uint8_t)m_features);
		// Serialize Commitment
		m_commitment.Serialize(serializer);

		m_hash = Crypto::Blake2b(serializer.GetBytes());
	}

	return m_hash;
}