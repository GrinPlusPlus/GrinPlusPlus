#pragma once

#include <Core/Models/OutputIdentifier.h>
#include <Core/Models/OutputLocation.h>
#include <Crypto/RangeProof.h>

// TODO: This is no longer just used for display purposes. It's now a core part of the wallet restore logic. Rename this and move it.
class OutputDisplayInfo
{
public:
	OutputDisplayInfo(const bool spent, const OutputIdentifier& identifier, const OutputLocation& location, const RangeProof& rangeProof)
		: m_spent(spent), m_identifier(identifier), m_location(location), m_rangeProof(rangeProof)
	{

	}

	inline bool IsSpent() const { return m_spent; }
	inline const OutputIdentifier& GetIdentifier() const { return m_identifier; }
	inline const OutputLocation& GetLocation() const { return m_location; }
	inline const RangeProof& GetRangeProof() const { return m_rangeProof; }
	
private:
	bool m_spent;
	OutputIdentifier m_identifier;
	OutputLocation m_location;
	RangeProof m_rangeProof;
};