#include <Core/Models/TransactionInput.h>

#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <Crypto/Crypto.h>

TransactionInput::TransactionInput(const EOutputFeatures features, Commitment&& commitment)
	: m_features(features), m_commitment(std::move(commitment))
{

}

void TransactionInput::Serialize(Serializer& serializer) const
{
	// Serialize OutputFeatures
	serializer.Append<uint8_t>((uint8_t)m_features);

	// Serialize Commitment
	m_commitment.Serialize(serializer);
}

TransactionInput TransactionInput::Deserialize(ByteBuffer& byteBuffer)
{
	// Read OutputFeatures (1 byte)
	const EOutputFeatures features = (EOutputFeatures)byteBuffer.ReadU8();

	// Read Commitment (33 bytes)
	Commitment commitment = Commitment::Deserialize(byteBuffer);

	return TransactionInput((EOutputFeatures)features, std::move(commitment));
}

Json::Value TransactionInput::ToJSON(const bool hex) const
{
	Json::Value inputNode;
	inputNode["features"] = OutputFeatures::ToString(GetFeatures());
	inputNode["commit"] = JsonUtil::ConvertToJSON(GetCommitment(), hex);
	return inputNode;
}

TransactionInput TransactionInput::FromJSON(const Json::Value& transactionInputJSON, const bool hex)
{
	const EOutputFeatures features = OutputFeatures::FromString(JsonUtil::GetRequiredField(transactionInputJSON, "features").asString());
	Commitment commitment = JsonUtil::GetCommitment(transactionInputJSON, "commit", hex);
	return TransactionInput(features, std::move(commitment));
}

const Hash& TransactionInput::GetHash() const
{
	if (m_hash == CBigInteger<32>())
	{
		Serializer serializer;
		Serialize(serializer);
		m_hash = Crypto::Blake2b(serializer.GetBytes());
	}

	return m_hash;
}