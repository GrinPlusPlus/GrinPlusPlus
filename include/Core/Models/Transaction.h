#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <Crypto/Hash.h>
#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Models/TransactionBody.h>
#include <Crypto/BlindingFactor.h>
#include <Core/Traits/Printable.h>
#include <json/json.h>
#include <mutex>
#include <memory>
#include <numeric>

////////////////////////////////////////
// TRANSACTION
////////////////////////////////////////
class Transaction  : public Traits::IPrintable
{
public:
	//
	// Constructors
	//
	Transaction(BlindingFactor&& offset, TransactionBody&& transactionBody);
	Transaction(const Transaction& transaction);
	Transaction(Transaction&& transaction) noexcept;
	Transaction() = default;

	//
	// Destructor
	//
	virtual ~Transaction() = default;

	//
	// Operators
	//
	Transaction& operator=(const Transaction& transaction);
	Transaction& operator=(Transaction&& transaction) noexcept = default;
	inline bool operator<(const Transaction& transaction) const { return GetHash() < transaction.GetHash(); }
	inline bool operator==(const Transaction& transaction) const { return GetHash() == transaction.GetHash(); }
	inline bool operator!=(const Transaction& transaction) const { return GetHash() != transaction.GetHash(); }

	//
	// Getters
	//
	const BlindingFactor& GetOffset() const { return m_offset; }
	const TransactionBody& GetBody() const { return m_transactionBody; }
	const std::vector<TransactionInput>& GetInputs() const { return m_transactionBody.GetInputs(); }
	const std::vector<TransactionOutput>& GetOutputs() const { return m_transactionBody.GetOutputs(); }
	const std::vector<TransactionKernel>& GetKernels() const { return m_transactionBody.GetKernels(); }

	uint64_t CalcFee() const noexcept { return m_transactionBody.CalcFee(); }
	uint8_t GetFeeShift() const noexcept { return m_transactionBody.GetFeeShift(); }

	bool FeeMeetsMinimum(const uint64_t block_height) const noexcept;

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

	//
	// Traits
	//
	std::string Format() const final { return GetHash().Format(); }

private:
	// The kernel "offset" k2 excess is k1G after splitting the key k = k1 + k2.
	BlindingFactor m_offset;

	// The transaction body.
	TransactionBody m_transactionBody;

	mutable Hash m_hash;
	mutable std::mutex m_mutex;
};

typedef std::shared_ptr<const Transaction> TransactionPtr;