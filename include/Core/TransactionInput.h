#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Core/Features.h>
#include <Crypto/Commitment.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

////////////////////////////////////////
// TRANSACTION INPUT
////////////////////////////////////////
class TransactionInput
{
public:
	//
	// Constructors
	//
	TransactionInput(const EOutputFeatures features, Commitment&& commitment);
	TransactionInput(const TransactionInput& transactionInput) = default;
	TransactionInput(TransactionInput&& transactionInput) noexcept = default;
	TransactionInput() = default;

	//
	// Destructor
	//
	~TransactionInput() = default;

	//
	// Operators
	//
	TransactionInput& operator=(const TransactionInput& transactionInput) = default;
	TransactionInput& operator=(TransactionInput&& transactionInput) noexcept = default;
	inline bool operator<(const TransactionInput& transactionInput) const { return Hash() < transactionInput.Hash(); }
	inline bool operator==(const TransactionInput& transactionInput) const { return Hash() == transactionInput.Hash(); }

	//
	// Getters
	//
	inline const EOutputFeatures GetFeatures() const { return m_features; }
	inline const Commitment& GetCommitment() const { return m_commitment; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static TransactionInput Deserialize(ByteBuffer& byteBuffer);

	//
	// Hashing
	//
	const CBigInteger<32>& Hash() const;

private:
	// The features of the output being spent. 
	// We will check maturity for coinbase output.
	EOutputFeatures m_features;

	// The commit referencing the output being spent.
	Commitment m_commitment;

	mutable CBigInteger<32> m_hash;
};