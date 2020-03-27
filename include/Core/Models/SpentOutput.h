#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Crypto/Commitment.h>
#include <Core/Models/OutputLocation.h>

class SpentOutput
{
public:
	//
	// Constructor
	//
	SpentOutput(const Commitment& commitment, const OutputLocation& outputLocation)
		: m_commitment(commitment), m_outputLocation(outputLocation) { }
	SpentOutput(Commitment&& commitment, OutputLocation&& outputLocation)
		: m_commitment(std::move(commitment)), m_outputLocation(std::move(outputLocation)) { }

	//
	// Getters
	//
	const Commitment& GetCommitment() const { return m_commitment; }
	const OutputLocation& GetLocation() const { return m_outputLocation; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		m_commitment.Serialize(serializer);
		m_outputLocation.Serialize(serializer);
	}

	static SpentOutput Deserialize(ByteBuffer& byteBuffer)
	{
		Commitment commitment = Commitment::Deserialize(byteBuffer);
		OutputLocation outputLocation = OutputLocation::Deserialize(byteBuffer);
		return SpentOutput(std::move(commitment), std::move(outputLocation));
	}

private:
	Commitment m_commitment;
	OutputLocation m_outputLocation;
};