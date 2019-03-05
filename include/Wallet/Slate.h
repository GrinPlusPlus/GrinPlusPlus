#pragma once

#include <Core/Models/Transaction.h>
#include <Wallet/ParticipantData.h>
#include <Core/Util/JsonUtil.h>
#include <json/json.h>
#include <stdint.h>

#ifdef NOMINMAX
#define SLATE_NOMINMAX_DEFINED
#undef NOMINMAX
#endif
#include <uuid.h>
#ifdef SLATE_NOMINMAX_DEFINED
#undef SLATE_NOMINMAX_DEFINED
#define NOMINMAX
#endif

// A 'Slate' is passed around to all parties to build up all of the public transaction data needed to create a finalized transaction. 
// Callers can pass the slate around by whatever means they choose, (but we can provide some binary or JSON serialization helpers here).
class Slate
{
public:
	Slate(const uint64_t version, const uint64_t numParticipants, uuids::uuid&& slateId, Transaction&& transaction, const uint64_t amount, const uint64_t fee, const uint64_t blockHeight, const uint64_t lockHeight)
		: m_version(version),
		m_numParticipants(numParticipants),
		m_slateId(std::move(slateId)),
		m_transaction(std::move(transaction)),
		m_amount(amount),
		m_fee(fee),
		m_blockHeight(blockHeight),
		m_lockHeight(lockHeight)
	{

	}

	inline const uuids::uuid& GetSlateId() const { return m_slateId; }
	inline uint64_t GetAmount() const { return m_amount; }
	inline uint64_t GetFee() const { return m_fee; }
	inline uint64_t GetLockHeight() const { return m_lockHeight; }
	inline uint64_t GetBlockHeight() const { return m_blockHeight; }
	inline const Transaction& GetTransaction() const { return m_transaction; }
	inline const std::vector<ParticipantData>& GetParticipantData() const { return m_participantData; }

	inline void AddParticpantData(const ParticipantData& participantData) { m_participantData.push_back(participantData); }

	Json::Value ToJSON() const
	{
		Json::Value slateNode;

		slateNode["version"] = m_version;
		slateNode["num_participants"] = m_numParticipants;
		slateNode["id"] = uuids::to_string(m_slateId);
		slateNode["amount"] = m_amount;
		slateNode["fee"] = m_fee;
		slateNode["height"] = m_blockHeight;
		slateNode["lock_height"] = m_lockHeight;
		slateNode["tx"] = m_transaction.ToJSON();

		Json::Value participantDataNode;
		for (const ParticipantData& participant : m_participantData)
		{
			participantDataNode.append(participant.ToJSON());
		}
		slateNode["participant_data"] = participantDataNode;

		return slateNode;
	}

	static Slate FromJSON(const Json::Value& slateNode)
	{
		const uint64_t version = JsonUtil::GetRequiredField(slateNode, "version").asUInt64();
		const uint64_t numParticipants = JsonUtil::GetRequiredField(slateNode, "num_participants").asUInt64();
		std::optional<uuids::uuid> slateIdOpt = uuids::uuid::from_string(JsonUtil::GetRequiredField(slateNode,"id").asString());
		if (!slateIdOpt.has_value())
		{
			throw DeserializationException();
		}

		const uint64_t amount = JsonUtil::GetRequiredField(slateNode, "amount").asUInt64();
		const uint64_t fee = JsonUtil::GetRequiredField(slateNode, "fee").asUInt64();
		const uint64_t height = JsonUtil::GetRequiredField(slateNode, "height").asUInt64();
		const uint64_t lockHeight = JsonUtil::GetRequiredField(slateNode, "lock_height").asUInt64();

		Transaction transaction = Transaction::FromJSON(JsonUtil::GetRequiredField(slateNode, "tx"));

		Slate slate(version, numParticipants, std::move(slateIdOpt.value()), std::move(transaction), amount, fee, height, lockHeight);

		const Json::Value participantDataNode = JsonUtil::GetRequiredField(slateNode, "participant_data");
		for (size_t i = 0; i < participantDataNode.size(); i++)
		{
			const Json::Value participantJSON = participantDataNode.get(Json::ArrayIndex(i), Json::nullValue);
			slate.AddParticpantData(ParticipantData::FromJSON(participantJSON));
		}

		return slate;
	}

private:
	// Slate format version
	uint64_t m_version;
	// The number of participants intended to take part in this transaction
	uint64_t m_numParticipants;
	// Unique transaction/slate ID, selected by sender
	uuids::uuid m_slateId;
	// The core transaction data:
	// inputs, outputs, kernels, kernel offset
	Transaction m_transaction;
	// base amount (excluding fee)
	uint64_t m_amount;
	// fee amount
	uint64_t m_fee;
	// Block height for the transaction
	uint64_t m_blockHeight;
	// Lock height
	uint64_t m_lockHeight;
	// Participant data, each participant in the transaction will insert their public data here.
	// For now, 0 is sender and 1 is receiver, though this will change for multi-party.
	std::vector<ParticipantData> m_participantData;
};