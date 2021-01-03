#include <Core/Models/Transaction.h>

#include <Consensus.h>
#include <Crypto/Hasher.h>
#include <Core/Global.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Config.h>

Transaction::Transaction(BlindingFactor&& offset, TransactionBody&& transactionBody)
	: m_offset(std::move(offset)), m_transactionBody(std::move(transactionBody)) { }

Transaction::Transaction(const Transaction& tx)
	: m_offset(tx.m_offset), m_transactionBody(tx.m_transactionBody), m_hash(tx.GetHash()) { }

Transaction::Transaction(Transaction&& tx) noexcept
	: m_offset(std::move(tx.m_offset)), m_transactionBody(std::move(tx.m_transactionBody)), m_hash(std::move(tx.m_hash)) { }

Transaction& Transaction::operator=(const Transaction& tx)
{
	m_offset = tx.m_offset;
	m_transactionBody = tx.m_transactionBody;
	m_hash = tx.GetHash();
	return *this;
}

void Transaction::Serialize(Serializer& serializer) const
{
	// Serialize BlindingFactor/Offset
	const CBigInteger<32>& offsetBytes = m_offset.GetBytes();
	serializer.AppendBigInteger(offsetBytes);

	// Serialize Transaction Body
	m_transactionBody.Serialize(serializer);
}

Transaction Transaction::Deserialize(ByteBuffer& byteBuffer)
{
	// Read BlindingFactor/Offset (32 bytes)
	BlindingFactor offset = BlindingFactor::Deserialize(byteBuffer);

	// Read Transaction Body (variable size)
	TransactionBody transactionBody = TransactionBody::Deserialize(byteBuffer);

	return Transaction(std::move(offset), std::move(transactionBody));
}

Json::Value Transaction::ToJSON() const
{
	Json::Value txNode;
	txNode["offset"] = JsonUtil::ConvertToJSON(GetOffset());
	txNode["body"] = GetBody().ToJSON();
	return txNode;
}

Transaction Transaction::FromJSON(const Json::Value& transactionJSON)
{
	Json::Value offsetJSON = JsonUtil::GetRequiredField(transactionJSON, "offset");
	BlindingFactor offset = JsonUtil::ConvertToBlindingFactor(offsetJSON);

	Json::Value bodyJSON = JsonUtil::GetRequiredField(transactionJSON, "body");
	TransactionBody body = TransactionBody::FromJSON(bodyJSON);

	return Transaction(std::move(offset), std::move(body));
}

const Hash& Transaction::GetHash() const
{
	std::unique_lock lock(m_mutex);

	if (m_hash == Hash{}) {
		Serializer serializer;
		Serialize(serializer);
		m_hash = Hasher::Blake2b(serializer.GetBytes());
	}

	return m_hash;
}

bool Transaction::FeeMeetsMinimum(const uint64_t block_height) const noexcept
{
	uint64_t fee_base = Global::GetConfig().GetFeeBase();
	uint64_t min_fee = fee_base * m_transactionBody.CalcWeight(block_height);

	return CalcFee() >= (min_fee << GetFeeShift());
}