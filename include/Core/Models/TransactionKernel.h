#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Traits/Committed.h>
#include <Core/Traits/Serializable.h>
#include <Core/Traits/Hashable.h>
#include <Crypto/Hash.h>
#include <Core/Models/Features.h>
#include <Core/Models/Fee.h>
#include <Crypto/Commitment.h>
#include <Crypto/Signature.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <json/json.h>

////////////////////////////////////////
// TRANSACTION KERNEL
////////////////////////////////////////
class TransactionKernel : public Traits::ICommitted, public Traits::IHashable, public Traits::ISerializable
{
public:
	//
	// Constructors
	//
	TransactionKernel(const EKernelFeatures features, const Fee& fee, const uint64_t lockHeight, Commitment&& excessCommitment, Signature&& excessSignature);
	TransactionKernel(const EKernelFeatures features, const Fee& fee, const uint64_t lockHeight, const Commitment& excessCom, const Signature& excessSig);
	TransactionKernel(const TransactionKernel& transactionKernel) = default;
	TransactionKernel(TransactionKernel&& transactionKernel) noexcept = default;
	TransactionKernel() = default;

	//
	// Destructor
	//
	virtual ~TransactionKernel() = default;

	//
	// Operators
	//
	TransactionKernel& operator=(const TransactionKernel& transactionKernel) = default;
	TransactionKernel& operator=(TransactionKernel&& transactionKernel) noexcept = default;
	bool operator<(const TransactionKernel& transactionKernel) const { return GetExcessCommitment() < transactionKernel.GetExcessCommitment(); }
	bool operator==(const TransactionKernel& transactionKernel) const { return GetHash() == transactionKernel.GetHash(); }
	bool operator!=(const TransactionKernel& transactionKernel) const { return GetHash() != transactionKernel.GetHash(); }

	//
	// Getters
	//
	EKernelFeatures GetFeatures() const { return m_features; }
	uint64_t GetFee() const { return m_fee.GetFee(); }
	uint8_t GetFeeShift() const noexcept { return m_fee.GetShift(); }
	uint64_t GetLockHeight() const { return m_lockHeight; }
	const Commitment& GetExcessCommitment() const { return m_excessCommitment; }
	const Signature& GetExcessSignature() const { return m_excessSignature; }
	bool IsCoinbase() const { return (m_features & EKernelFeatures::COINBASE_KERNEL) == EKernelFeatures::COINBASE_KERNEL; }
	Hash GetSignatureMessage() const;
	static Hash GetSignatureMessage(const EKernelFeatures features, const Fee& fee, const uint64_t lockHeight);

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const final;
	static TransactionKernel Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON() const;
	static TransactionKernel FromJSON(const Json::Value& transactionKernelJSON);

	//
	// Traits
	//
	const Hash& GetHash() const final { return m_hash; }
	const Commitment& GetCommitment() const final { return m_excessCommitment; }

private:
	// Options for a kernel's structure or use
	EKernelFeatures m_features;

	// Fee originally included in the transaction this proof is for.
	Fee m_fee;

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