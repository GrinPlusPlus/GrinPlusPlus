#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Traits/Serializable.h>
#include <Crypto/Models/Commitment.h>
#include <Core/Models/OutputLocation.h>
#include <cstdint>
#include <unordered_map>

class SpentOutput : public Traits::ISerializable
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
	void Serialize(Serializer& serializer) const final
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

class SpentOutputs : public Traits::ISerializable
{
public:
	SpentOutputs(std::vector<SpentOutput>&& spentOutputs)
		: m_spentOutputs(std::move(spentOutputs)) { }
	SpentOutputs(const std::vector<SpentOutput>& spentOutputs)
		: m_spentOutputs(spentOutputs) { }


	//
	// Getters
	//
	const std::vector<SpentOutput>& GetSpentOutputs() const noexcept { return m_spentOutputs; }
	std::unordered_map<Commitment, OutputLocation> BuildMap() const noexcept
	{
		std::unordered_map<Commitment, OutputLocation> spentOutputs;
		spentOutputs.reserve(m_spentOutputs.size());

		for (const SpentOutput& output : m_spentOutputs)
		{
			spentOutputs.insert({ output.GetCommitment(), output.GetLocation() });
		}

		return spentOutputs;
	}

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const final
	{
		serializer.Append<uint8_t>(/* version= */ 0);
		serializer.Append<uint16_t>((uint16_t)m_spentOutputs.size());

		for (const SpentOutput& output : m_spentOutputs)
		{
			output.Serialize(serializer);
		}
	}

	static SpentOutputs Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t version = byteBuffer.ReadU8();
		assert(version == 0);

		const uint16_t numOutputs = byteBuffer.ReadU16();

		std::vector<SpentOutput> spentOutputs;
		spentOutputs.reserve(numOutputs);

		for (uint16_t i = 0; i < numOutputs; i++)
		{
			spentOutputs.push_back(SpentOutput::Deserialize(byteBuffer));
		}

		return SpentOutputs(std::move(spentOutputs));
	}

private:
	std::vector<SpentOutput> m_spentOutputs;
};