#pragma once

#include <Wallet/Models/SlateVersionInfo.h>
#include <Wallet/Models/SlatePaymentProof.h>
#include <Wallet/ParticipantData.h>
#include <Core/Models/Transaction.h>
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

static uint64_t MIN_SLATE_VERSION = 2;
static uint64_t MAX_SLATE_VERSION = 3;

// A 'Slate' is passed around to all parties to build up all of the public transaction data needed to create a finalized transaction. 
// Callers can pass the slate around by whatever means they choose, (but we can provide some binary or JSON serialization helpers here).
class Slate
{
public:
	Slate(
		SlateVersionInfo&& versionInfo,
		const uint64_t numParticipants,
		uuids::uuid&& slateId,
		Transaction&& transaction,
		const uint64_t amount,
		const uint64_t fee,
		const uint64_t blockHeight,
		const uint64_t lockHeight,
		const std::optional<SlatePaymentProof>& proofOpt)
		: m_versionInfo(versionInfo),
		m_numParticipants(numParticipants),
		m_slateId(std::move(slateId)),
		m_transaction(std::move(transaction)),
		m_amount(amount),
		m_fee(fee),
		m_blockHeight(blockHeight),
		m_lockHeight(lockHeight),
		m_proofOpt(proofOpt)
	{

	}

	const uuids::uuid& GetSlateId() const { return m_slateId; }
	const SlateVersionInfo& GetVersionInfo() const { return m_versionInfo; }
	uint64_t GetAmount() const { return m_amount; }
	uint64_t GetFee() const { return m_fee; }
	uint64_t GetLockHeight() const { return m_lockHeight; }
	uint64_t GetBlockHeight() const { return m_blockHeight; }
	const Transaction& GetTransaction() const { return m_transaction; }
	const std::vector<ParticipantData>& GetParticipantData() const { return m_participantData; }
	std::vector<ParticipantData>& GetParticipantData() { return m_participantData; }

	void AddParticpantData(const ParticipantData& participantData) { m_participantData.push_back(participantData); }
	void UpdateTransaction(const Transaction& transaction) { m_transaction = transaction; }

	Json::Value ToJSON() const
	{
		Json::Value slateNode;
		slateNode["version_info"] = m_versionInfo.ToJSON();
		if (m_versionInfo.GetVersion() >= 3)
		{
			slateNode["ttl_cutoff_height"] = Json::Value(Json::nullValue); // TODO: Implement
			slateNode["payment_proof"] = m_proofOpt.has_value() ? m_proofOpt.value().ToJSON() : Json::Value(Json::nullValue);
		}

		slateNode["num_participants"] = m_numParticipants;
		slateNode["id"] = uuids::to_string(m_slateId);
		slateNode["amount"] = std::to_string(m_amount);
		slateNode["fee"] = std::to_string(m_fee);
		slateNode["height"] = std::to_string(m_blockHeight);
		slateNode["lock_height"] = std::to_string(m_lockHeight);
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
		// Version info
		const uint16_t version = (uint16_t)slateNode.get("version", Json::Value(0)).asUInt();
		const Json::Value versionInfoJSON = slateNode.get("version_info", Json::nullValue);
		SlateVersionInfo versionInfo = (versionInfoJSON != Json::nullValue) ? SlateVersionInfo::FromJSON(versionInfoJSON) : SlateVersionInfo(version);

		const uint64_t numParticipants = JsonUtil::GetRequiredUInt64(slateNode, "num_participants");
		std::optional<uuids::uuid> slateIdOpt = uuids::uuid::from_string(JsonUtil::GetRequiredField(slateNode,"id").asString());
		if (!slateIdOpt.has_value())
		{
			throw DESERIALIZATION_EXCEPTION();
		}

		const uint64_t amount = JsonUtil::GetRequiredUInt64(slateNode, "amount");
		const uint64_t fee = JsonUtil::GetRequiredUInt64(slateNode, "fee");
		const uint64_t height = JsonUtil::GetRequiredUInt64(slateNode, "height");
		const uint64_t lockHeight = JsonUtil::GetRequiredUInt64(slateNode, "lock_height");

		Transaction transaction = Transaction::FromJSON(JsonUtil::GetRequiredField(slateNode, "tx"));

		std::optional<SlatePaymentProof> proofOpt = std::nullopt;
		std::optional<Json::Value> proofJsonOpt = JsonUtil::GetOptionalField(slateNode, "payment_proof");
		if (proofJsonOpt.has_value() && !proofJsonOpt.value().isNull())
		{
			proofOpt = std::make_optional<SlatePaymentProof>(SlatePaymentProof::FromJSON(proofJsonOpt.value()));
		}

		Slate slate(
			std::move(versionInfo),
			numParticipants,
			std::move(slateIdOpt.value()),
			std::move(transaction),
			amount,
			fee,
			height,
			lockHeight,
			proofOpt
		);

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
	SlateVersionInfo m_versionInfo;
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

	std::optional<SlatePaymentProof> m_proofOpt;
};