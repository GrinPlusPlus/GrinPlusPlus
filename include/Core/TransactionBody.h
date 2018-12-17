#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <vector>
#include <BigInteger.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>
#include <Core/TransactionInput.h>
#include <Core/TransactionOutput.h>
#include <Core/TransactionKernel.h>

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

private:
	// List of inputs spent by the transaction.
	std::vector<TransactionInput> m_inputs;

	// List of outputs the transaction produces.
	std::vector<TransactionOutput> m_outputs;

	// List of kernels that make up this transaction (usually a single kernel).
	std::vector<TransactionKernel> m_kernels;
};