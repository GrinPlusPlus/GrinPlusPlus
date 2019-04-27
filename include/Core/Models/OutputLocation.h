#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>

class OutputLocation
{
public:
	//
	// Constructor
	//
	OutputLocation(const uint64_t mmrIndex, const uint64_t blockHeight)
		: m_mmrIndex(mmrIndex), m_blockHeight(blockHeight)
	{

	}

	//
	// Getters
	//
	inline const uint64_t GetMMRIndex() const { return m_mmrIndex; }
	inline const uint64_t GetBlockHeight() const { return m_blockHeight; }

	//
	// Serialization/Deserialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.Append(m_mmrIndex);
		serializer.Append(m_blockHeight);
	}

	static OutputLocation Deserialize(ByteBuffer& byteBuffer)
	{
		const uint64_t mmrIndex = byteBuffer.ReadU64();
		const uint64_t blockHeight = byteBuffer.ReadU64();
		return OutputLocation(mmrIndex, blockHeight);
	}

private:
	uint64_t m_mmrIndex;
	uint64_t m_blockHeight;
};