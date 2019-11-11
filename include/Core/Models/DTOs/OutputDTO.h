#pragma once

#include <Core/Models/OutputIdentifier.h>
#include <Core/Models/OutputLocation.h>
#include <Crypto/RangeProof.h>

class OutputDTO
{
public:
	OutputDTO(
		const bool spent,
		const OutputIdentifier& identifier,
		const OutputLocation& location,
		const RangeProof& rangeProof)
		: m_spent(spent),
		m_identifier(identifier),
		m_location(location),
		m_rangeProof(rangeProof)
	{

	}

	bool IsSpent() const { return m_spent; }
	const OutputIdentifier& GetIdentifier() const { return m_identifier; }
	const OutputLocation& GetLocation() const { return m_location; }
	const RangeProof& GetRangeProof() const { return m_rangeProof; }
	
private:
	bool m_spent;
	OutputIdentifier m_identifier;
	OutputLocation m_location;
	RangeProof m_rangeProof;
};