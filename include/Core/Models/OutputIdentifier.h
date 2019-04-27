#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Models/Features.h>
#include <Crypto/Commitment.h>

// Forward Declarations
class TransactionOutput;

class OutputIdentifier
{
public:
	//
	// Constructors
	//
	OutputIdentifier(const EOutputFeatures features, Commitment&& commitment);
	OutputIdentifier(const OutputIdentifier& outputIdentifier) = default;
	OutputIdentifier(OutputIdentifier&& outputIdentifier) noexcept = default;
	OutputIdentifier() = default;

	static OutputIdentifier FromOutput(const TransactionOutput& output);

	//
	// Destructor
	//
	~OutputIdentifier() = default;

	//
	// Operators
	//
	OutputIdentifier& operator=(const OutputIdentifier& outputIdentifier) = default;
	OutputIdentifier& operator=(OutputIdentifier&& outputIdentifier) noexcept = default;
	inline bool operator<(const OutputIdentifier& outputIdentifier) const { return GetCommitment() < outputIdentifier.GetCommitment(); }
	inline bool operator==(const OutputIdentifier& outputIdentifier) const { return GetCommitment() == outputIdentifier.GetCommitment(); }

	//
	// Getters
	//
	inline const EOutputFeatures GetFeatures() const { return m_features; }
	inline const Commitment& GetCommitment() const { return m_commitment; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const;
	static OutputIdentifier Deserialize(ByteBuffer& byteBuffer);

private:
	EOutputFeatures m_features;
	Commitment m_commitment;
};