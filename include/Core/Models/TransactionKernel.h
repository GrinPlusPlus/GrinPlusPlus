#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Crypto/Hash.h>
#include <Core/Models/Features.h>
#include <Crypto/Commitment.h>
#include <Crypto/Signature.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <json/json.h>

////////////////////////////////////////
// TRANSACTION KERNEL
////////////////////////////////////////
class TransactionKernel
{
public:
	//
	// Constructors
	//
	TransactionKernel(const EKernelFeatures features, const uint64_t fee, const uint64_t lockHeight, Commitment&& excessCommitment, Signature&& excessSignature);
	TransactionKernel(const TransactionKernel& transactionKernel) = default;
	TransactionKernel(TransactionKernel&& transactionKernel) noexcept = default;
	TransactionKernel() = default;

	//
	// Destructor
	//
	~TransactionKernel() = default;

	//
	// Operators
	//
	TransactionKernel& operator=(const TransactionKernel& transactionKernel) = default;
	TransactionKernel& operator=(TransactionKernel&& transactionKernel) noexcept = default;
	inline bool operator<(const TransactionKernel& transactionKernel) const { return GetExcessCommitment() < transactionKernel.GetExcessCommitment(); }
	inline bool operator==(const TransactionKernel& transactionKernel) const { return GetHash() == transactionKernel.GetHash(); }
	inline bool operator!=(const TransactionKernel& transactionKernel) const { return GetHash() != transactionKernel.GetHash(); }

	//
	// Getters
	//
	inline const EKernelFeatures GetFeatures() const { return m_features; }
	inline const uint64_t GetFee() const { return m_fee; }
	inline const uint64_t GetLockHeight() const { return m_lockHeight; }
	inline const Commitment& GetExcessCommitment() const { return m_excessCommitment; }
	inline const Signature& GetExcessSignature() const { return m_excessSignature; }
	Hash GetSignatureMessage() const;

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static TransactionKernel Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON(const bool hex) const;
	static TransactionKernel FromJSON(const Json::Value& transactionKernelJSON, const bool hex);

	//
	// Hashing
	//
	const Hash& GetHash() const;

private:
	// Options for a kernel's structure or use
	EKernelFeatures m_features;

	// Fee originally included in the transaction this proof is for.
	uint64_t m_fee;

	// This kernel is not valid earlier than m_lockHeight blocks
	// The max m_lockHeight of all *inputs* to this transaction
	uint64_t m_lockHeight;

	// Remainder of the sum of all transaction commitments. 
	// If the transaction is well formed, amounts components should sum to zero and the excess is hence a valid public key.
	Commitment m_excessCommitment;

	// The signature proving the excess is a valid public key, which signs the transaction fee.
	Signature m_excessSignature;

	mutable Hash m_hash;
};

static struct
{
	bool operator()(const TransactionKernel& a, const TransactionKernel& b) const
	{
		return a.GetHash() < b.GetHash();
	}
} SortKernelsByHash;