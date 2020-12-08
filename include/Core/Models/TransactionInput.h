#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Traits/Committed.h>
#include <Core/Traits/Hashable.h>
#include <Crypto/Hash.h>
#include <Core/Models/Features.h>
#include <Crypto/Commitment.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <json/json.h>

////////////////////////////////////////
// TRANSACTION INPUT
////////////////////////////////////////
class TransactionInput : public Traits::ICommitted, Traits::IHashable
{
public:
	//
	// Constructors
	//
	TransactionInput(Commitment&& commitment);
	TransactionInput(const Commitment& commitment)
		: TransactionInput(Commitment(commitment)) {  }
	TransactionInput(const TransactionInput& input) = default;
	TransactionInput(TransactionInput&& input) noexcept = default;
	TransactionInput() = default;

	//
	// Destructor
	//
	virtual ~TransactionInput() = default;

	//
	// Operators
	//
	TransactionInput& operator=(const TransactionInput& rhs) = default;
	TransactionInput& operator=(TransactionInput&& rhs) noexcept = default;
	bool operator<(const TransactionInput& rhs) const { return GetCommitment() < rhs.GetCommitment(); }
	bool operator==(const TransactionInput& rhs) const { return GetCommitment() == rhs.GetCommitment(); }

	//
	// Getters
	//
	const Commitment& GetCommitment() const final { return m_commitment; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static TransactionInput Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON() const;
	static TransactionInput FromJSON(const Json::Value& transactionInputJSON);

	//
	// Traits
	//
	const Hash& GetHash() const final { return m_hash; }

private:
	// The commit referencing the output being spent.
	Commitment m_commitment;

	mutable Hash m_hash;
};

static struct
{
	bool operator()(const TransactionInput& a, const TransactionInput& b) const
	{
		return a.GetHash() < b.GetHash();
	}
} SortInputsByHash;