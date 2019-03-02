#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Crypto/Hash.h>
#include <Core/Models/Features.h>
#include <Core/Models/OutputIdentifier.h>
#include <Crypto/Commitment.h>
#include <Crypto/RangeProof.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <json/json.h>

////////////////////////////////////////
// TRANSACTION OUTPUT
////////////////////////////////////////
class TransactionOutput
{
public:
	//
	// Constructors
	//
	TransactionOutput(const EOutputFeatures features, Commitment&& commitment, RangeProof&& rangeProof);
	TransactionOutput(const TransactionOutput& transactionOutput) = default;
	TransactionOutput(TransactionOutput&& transactionOutput) noexcept = default;
	TransactionOutput() = default;

	//
	// Destructor
	//
	~TransactionOutput() = default;

	//
	// Operators
	//
	TransactionOutput& operator=(const TransactionOutput& transactionOutput) = default;
	TransactionOutput& operator=(TransactionOutput&& transactionOutput) noexcept = default;
	inline bool operator<(const TransactionOutput& transactionOutput) const { return GetCommitment() < transactionOutput.GetCommitment(); }
	inline bool operator==(const TransactionOutput& transactionOutput) const { return GetHash() == transactionOutput.GetHash(); }

	//
	// Getters
	//
	inline const EOutputFeatures GetFeatures() const { return m_features; }
	inline const Commitment& GetCommitment() const { return m_commitment; }
	inline const RangeProof& GetRangeProof() const { return m_rangeProof; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static TransactionOutput Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON() const;
	static TransactionOutput FromJSON(const Json::Value& transactionOutputJSON);

	//
	// Hashing
	//
	const Hash& GetHash() const;

private:
	// Options for an output's structure or use
	EOutputFeatures m_features;

	// The homomorphic commitment representing the output amount
	Commitment m_commitment;

	// A proof that the commitment is in the right range
	RangeProof m_rangeProof;

	mutable Hash m_hash;
};

static struct
{
	bool operator()(const TransactionOutput& a, const TransactionOutput& b) const
	{
		return a.GetHash() < b.GetHash();
	}
} SortOutputsByHash;