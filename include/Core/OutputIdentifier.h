#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <Core/Features.h>
#include <Crypto/Commitment.h>

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