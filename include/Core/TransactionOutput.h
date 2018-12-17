#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Core/Features.h>
#include <Core/OutputIdentifier.h>
#include <Crypto/Commitment.h>
#include <Crypto/RangeProof.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

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
	inline bool operator<(const TransactionOutput& transactionOutput) const { return Hash() < transactionOutput.Hash(); }
	inline bool operator==(const TransactionOutput& transactionOutput) const { return Hash() == transactionOutput.Hash(); }

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

	//
	// Hashing
	//
	const CBigInteger<32>& Hash() const;

private:
	// Options for an output's structure or use
	EOutputFeatures m_features;

	// The homomorphic commitment representing the output amount
	Commitment m_commitment;

	// A proof that the commitment is in the right range
	RangeProof m_rangeProof;

	mutable CBigInteger<32> m_hash;
};