#include <Core/Models/TransactionInput.h>

#include <Core/Global.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <Crypto/Hasher.h>
#include <BlockChain/ICoinView.h>

TransactionInput::TransactionInput(Commitment&& commitment)
	: m_commitment(std::move(commitment))
{
		Serializer serializer(ProtocolVersion::Local()); // TODO: Need to fix sort for V2 peers
		Serialize(serializer);
		m_hash = Hasher::Blake2b(serializer.GetBytes());
}

void TransactionInput::Serialize(Serializer& serializer) const
{
	if (serializer.GetProtocolVersion() < EProtocolVersion::V3) {
		EOutputFeatures features = Global::GetCoinView()->GetOutputType(m_commitment);
		serializer.Append<uint8_t>((uint8_t)features);
	}

	m_commitment.Serialize(serializer);
}

TransactionInput TransactionInput::Deserialize(ByteBuffer& byteBuffer)
{
	if (byteBuffer.GetProtocolVersion() < EProtocolVersion::V3) {
		byteBuffer.ReadU8(); // Ignore features byte
	}

	Commitment commitment = Commitment::Deserialize(byteBuffer);

	return TransactionInput(std::move(commitment));
}

Json::Value TransactionInput::ToJSON() const
{
	Json::Value inputNode;
	inputNode["commit"] = JsonUtil::ConvertToJSON(GetCommitment());
	return inputNode;
}

TransactionInput TransactionInput::FromJSON(const Json::Value& transactionInputJSON)
{
	Commitment commitment = JsonUtil::GetCommitment(transactionInputJSON, "commit");
	return TransactionInput(std::move(commitment));
}