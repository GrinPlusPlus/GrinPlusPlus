#include <Core/Models/TransactionOutput.h>
#include <Core/Util/JsonUtil.h>
#include <Crypto/Hasher.h>

TransactionOutput::TransactionOutput(const EOutputFeatures features, Commitment commitment, RangeProof rangeProof)
	: m_features(features), m_commitment(std::move(commitment)), m_rangeProof(std::move(rangeProof))
{
		Serializer serializer;
		// Serialize OutputFeatures
		serializer.Append<uint8_t>((uint8_t)m_features);

		// Serialize Commitment
		m_commitment.Serialize(serializer);
		m_hash = Hasher::Blake2b(serializer.GetBytes());
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

Json::Value TransactionOutput::ToJSON() const
{
	Json::Value outputNode;
	outputNode["features"] = OutputFeatures::ToString(GetFeatures());
	outputNode["commit"] = JsonUtil::ConvertToJSON(GetCommitment());
	outputNode["proof"] = JsonUtil::ConvertToJSON(GetRangeProof());
	return outputNode;
}

TransactionOutput TransactionOutput::FromJSON(const Json::Value& transactionOutputJSON)
{
	const EOutputFeatures features = OutputFeatures::FromString(JsonUtil::GetRequiredString(transactionOutputJSON, "features"));
	Commitment commitment = JsonUtil::GetCommitment(transactionOutputJSON, "commit");
	RangeProof rangeProof = JsonUtil::GetRangeProof(transactionOutputJSON, "proof");
	return TransactionOutput(features, std::move(commitment), std::move(rangeProof));
}