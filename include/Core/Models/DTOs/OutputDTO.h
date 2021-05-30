#pragma once

#include <Core/Models/OutputIdentifier.h>
#include <Core/Models/OutputLocation.h>
#include <Core/Models/TransactionOutput.h>
#include <Crypto/Models/RangeProof.h>
#include <Crypto/Hasher.h>
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
		m_rangeProof(rangeProof) { }

	OutputDTO(
		const bool spent,
		OutputIdentifier&& identifier,
		OutputLocation&& location,
		RangeProof&& rangeProof)
		: m_spent(spent),
		m_identifier(std::move(identifier)),
		m_location(std::move(location)),
		m_rangeProof(std::move(rangeProof)) { }

	bool IsSpent() const { return m_spent; }
	const OutputIdentifier& GetIdentifier() const noexcept { return m_identifier; }
	EOutputFeatures GetFeatures() const noexcept { return m_identifier.GetFeatures(); }
	const Commitment& GetCommitment() const noexcept { return m_identifier.GetCommitment(); }
	const OutputLocation& GetLocation() const noexcept { return m_location; }
	uint64_t GetBlockHeight() const noexcept { return m_location.GetBlockHeight(); }
	uint64_t GetMMRPosition() const noexcept { return m_location.GetPosition(); }
	const LeafIndex& GetLeafIndex() const noexcept { return m_location.GetLeafIndex(); }
	const RangeProof& GetRangeProof() const noexcept { return m_rangeProof; }

	TransactionOutput ToTxOutput() const noexcept {
		return TransactionOutput{ GetFeatures(), GetCommitment(), GetRangeProof() };
	}

	Json::Value ToJSON() const
	{
		Json::Value json;
		json["output_type"] = OutputFeatures::ToString(m_identifier.GetFeatures());
		json["commit"] = m_identifier.GetCommitment().ToHex();
		json["spent"] = false;
		json["proof"] = m_rangeProof.Format();

		Serializer proofSerializer;
		m_rangeProof.Serialize(proofSerializer);
		json["proof_hash"] = Hasher::Blake2b(proofSerializer.GetBytes()).ToHex();

		json["block_height"] = m_location.GetBlockHeight();
		json["merkle_proof"] = Json::nullValue;
		json["mmr_index"] = m_location.GetLeafIndex().GetPosition() + 1;
		return json;
	}

	static OutputDTO FromJSON(const Json::Value& json)
	{
		bool spent = JsonUtil::GetRequiredBool(json, "spent");

		// OutputIdentifier
		EOutputFeatures features = OutputFeatures::FromString(JsonUtil::GetRequiredString(json, "output_type"));
		Commitment commit = JsonUtil::GetCommitment(json, "commit");

		// RangeProof
		RangeProof proof = JsonUtil::GetRangeProof(json, "proof");

		return OutputDTO(
			spent,
			OutputIdentifier(features, std::move(commit)),
			OutputLocation::FromJSON(json),
			std::move(proof)
		);
	}
	
private:
	bool m_spent;
	OutputIdentifier m_identifier;
	OutputLocation m_location;
	RangeProof m_rangeProof;
};