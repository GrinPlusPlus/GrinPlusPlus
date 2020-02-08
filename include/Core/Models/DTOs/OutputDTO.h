#pragma once

#include <Core/Models/OutputIdentifier.h>
#include <Core/Models/OutputLocation.h>
#include <Crypto/RangeProof.h>
#include <Crypto/Crypto.h>
#include <json/json.h>

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

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["output_type"] = m_identifier.GetFeatures() == DEFAULT_OUTPUT ? "Transaction" : "Coinbase";
		json["commit"] = m_identifier.GetCommitment().ToHex();
		json["spent"] = false;
		json["proof"] = m_rangeProof.Format();

		Serializer proofSerializer;
		m_rangeProof.Serialize(proofSerializer);
		json["proof_hash"] = Crypto::Blake2b(proofSerializer.GetBytes()).ToHex();

		json["block_height"] = m_location.GetBlockHeight();
		json["merkle_proof"] = Json::nullValue;
		json["mmr_index"] = m_location.GetMMRIndex() + 1;
		return json;
	}
	
private:
	bool m_spent;
	OutputIdentifier m_identifier;
	OutputLocation m_location;
	RangeProof m_rangeProof;
};