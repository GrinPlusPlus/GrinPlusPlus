#include <Core/Models/TransactionInput.h>

#include <Core/Global.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <Crypto/Hasher.h>
#include <BlockChain/ICoinView.h>

TransactionInput::TransactionInput(const EOutputFeatures features, Commitment&& commitment)
	: m_features(features), m_commitment(std::move(commitment))
{
		Serializer serializer(ProtocolVersion::Local());
		Serialize(serializer);
		m_hash = Hasher::Blake2b(serializer.GetBytes());
}

void TransactionInput::Serialize(Serializer& serializer) const
{
	if (serializer.GetProtocolVersion() < EProtocolVersion::V3) {
		serializer.Append<uint8_t>((uint8_t)m_features);
	}

	m_commitment.Serialize(serializer);
}

TransactionInput TransactionInput::Deserialize(ByteBuffer& byteBuffer)
{
	if (byteBuffer.GetProtocolVersion() >= EProtocolVersion::V3) {
		Commitment commitment = Commitment::Deserialize(byteBuffer);
		EOutputFeatures features = Global::GetCoinView()->GetOutputType(commitment);
		return TransactionInput(features, std::move(commitment));
	} else {
		EOutputFeatures features = (EOutputFeatures)byteBuffer.ReadU8();
		Commitment commitment = Commitment::Deserialize(byteBuffer);
		return TransactionInput(features, std::move(commitment));
	}
}

Json::Value TransactionInput::ToJSON() const
{
	Json::Value inputNode;
	inputNode["features"] = OutputFeatures::ToString(m_features);
	inputNode["commit"] = JsonUtil::ConvertToJSON(GetCommitment());
	return inputNode;
}

TransactionInput TransactionInput::FromJSON(const Json::Value& transactionInputJSON)
{
	EOutputFeatures features = OutputFeatures::FromString(JsonUtil::GetRequiredString(transactionInputJSON, "features"));
	Commitment commitment = JsonUtil::GetCommitment(transactionInputJSON, "commit");
	return TransactionInput(features, std::move(commitment));
}