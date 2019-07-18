#pragma once

#include <stdint.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>

class Capabilities
{
public:
	enum ECapability
	{
		// We don't know (yet) what the peer can do.
		UNKNOWN = 0x00,

		// Full archival node, has the whole history without any pruning.
		FULL_HIST = 0x01,

		// Can provide block headers and the TxHashSet for some recent-enough height.
		TXHASHET_HIST = 0x02, // LIGHT_CLIENT: We can prune old range-proofs and drop unnecessary rangeproof hashes if this isn't set

		// Can provide a list of healthy peers
		PEER_LIST = 0x04,

		FAST_SYNC_NODE = (TXHASHET_HIST | PEER_LIST),

		ARCHIVE_NODE = (FULL_HIST | TXHASHET_HIST | PEER_LIST)
	};

	Capabilities(const uint32_t value)
		: m_value(value)
	{

	}

	Capabilities(const ECapability value)
		: m_value(value)
	{

	}

	inline bool IsUnknown() const { return m_value == UNKNOWN; }
	inline bool HasCapability(const ECapability capability) const { return (m_value & (uint32_t)capability) == (uint32_t)capability; }
	inline void AddCapability(const ECapability capability) { m_value = (m_value | capability); }
	inline ECapability GetCapability() const { return (ECapability)m_value; }

	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint32_t>(m_value);
	}

	static Capabilities Deserialize(ByteBuffer& byteBuffer)
	{
		const uint32_t value = byteBuffer.ReadU32();
		return Capabilities(value);
	}

private:
	uint32_t m_value;
};