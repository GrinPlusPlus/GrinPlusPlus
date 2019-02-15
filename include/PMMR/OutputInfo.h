#pragma once

#include <Core/OutputIdentifier.h>
#include <Core/OutputLocation.h>
#include <Crypto/RangeProof.h>

class OutputInfo
{
public:
	OutputInfo(const bool spent, const OutputIdentifier& identifier, const OutputLocation& location, const RangeProof& rangeProof)
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