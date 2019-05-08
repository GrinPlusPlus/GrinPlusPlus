#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <vector>
#include <Crypto/BigInteger.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Models/TransactionInput.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Models/TransactionKernel.h>
#include <json/json.h>

////////////////////////////////////////
// TRANSACTION BODY
////////////////////////////////////////
class TransactionBody
{
public:
	//
	// Constructors
	//
	TransactionBody(std::vector<TransactionInput>&& inputs, std::vector<TransactionOutput>&& outputs, std::vector<TransactionKernel>&& kernels);
	TransactionBody(const TransactionBody& other) = default;
	TransactionBody(TransactionBody&& other) noexcept = default;
	TransactionBody() = default;

	//
	// Destructor
	//
	~TransactionBody() = default;

	//
	// Operators
	//
	TransactionBody& operator=(const TransactionBody& other) = default;
	TransactionBody& operator=(TransactionBody&& other) noexcept = default;

	//
	// Getters
	//
	inline const std::vector<TransactionInput>& GetInputs() const { return m_inputs; }
	inline const std::vector<TransactionOutput>& GetOutputs() const { return m_outputs; }
	inline const std::vector<TransactionKernel>& GetKernels() const { return m_kernels; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static TransactionBody Deserialize(ByteBuffer& byteBuffer);
	Json::Value ToJSON(const bool hex) const;
	static TransactionBody FromJSON(const Json::Value& transactionBodyJSON, const bool hex);

private:
	// List of inputs spent by the transaction.
	std::vector<TransactionInput> m_inputs;

	// List of outputs the transaction produces.
	std::vector<TransactionOutput> m_outputs;

	// List of kernels that make up this transaction (usually a single kernel).
	std::vector<TransactionKernel> m_kernels;
};