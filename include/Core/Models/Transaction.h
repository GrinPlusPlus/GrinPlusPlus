#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <vector>
#include <Crypto/Hash.h>
#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Models/TransactionBody.h>
#include <Crypto/BlindingFactor.h>
#include <json/json.h>

////////////////////////////////////////
// TRANSACTION
////////////////////////////////////////
class Transaction
{
public:
	//
	// Constructors
	//
	Transaction(BlindingFactor&& offset, TransactionBody&& transactionBody);
	Transaction(const Transaction& transaction) = default;
	Transaction(Transaction&& transaction) noexcept = default;
	Transaction() = default;

	//
	// Destructor
	//
	~Transaction() = default;

	//
	// Operators
	//
	Transaction& operator=(const Transaction& transaction) = default;
	Transaction& operator=(Transaction&& transaction) noexcept = default;
	inline bool operator<(const Transaction& transaction) const { return GetHash() < transaction.GetHash(); }
	inline bool operator==(const Transaction& transaction) const { return GetHash() == transaction.GetHash(); }
	inline bool operator!=(const Transaction& transaction) const { return GetHash() != transaction.GetHash(); }

	//
	// Getters
	//
	inline const BlindingFactor& GetOffset() const { return m_offset; }
	inline const TransactionBody& GetBody() const { return m_transactionBody; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static Transaction Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON() const;
	static Transaction FromJSON(const Json::Value& transactionJSON);

	//
	// Hashing
	//
	const Hash& GetHash() const;

private:
	// The kernel "offset" k2 excess is k1G after splitting the key k = k1 + k2.
	BlindingFactor m_offset;

	// The transaction body.
	TransactionBody m_transactionBody;

	mutable Hash m_hash;
};
