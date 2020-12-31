#pragma once

#include <Wallet/Models/Slate/SlateCommitment.h>
#include <Wallet/Models/Slate/SlatePaymentProof.h>
#include <Wallet/Models/Slate/SlateFeatureArgs.h>
#include <Wallet/Models/Slate/SlateSignature.h>
#include <Wallet/Models/Slate/SlateStage.h>
#include <Core/Models/Transaction.h>
#include <Core/Traits/Printable.h>
#include <json/json.h>
#include <cstdint>

#ifdef NOMINMAX
#define SLATE_NOMINMAX_DEFINED
#undef NOMINMAX
#endif
#include <uuid.h>
#ifdef SLATE_NOMINMAX_DEFINED
#undef SLATE_NOMINMAX_DEFINED
#define NOMINMAX
#endif

static uint16_t MIN_SLATE_VERSION = 4;
static uint16_t MAX_SLATE_VERSION = 4;

// A 'Slate' is passed around to all parties to build up all of the public transaction data needed to create a finalized transaction. 
// Callers can pass the slate around by whatever means they choose, (but we can provide some binary or JSON serialization helpers here).
class Slate : public Traits::IPrintable
{
public:
	uuids::uuid slateId;
	SlateStage stage;
	uint16_t version;
	uint16_t blockVersion;
	uint8_t numParticipants;

	// Time to Live, or block height beyond which wallets should refuse to further process the transaction.
	// Assumed 0 (no ttl) if omitted from the slate. To be used when delayed transaction posting is desired.
	uint64_t ttl;

	EKernelFeatures kernelFeatures;
	std::optional<SlateFeatureArgs> featureArgsOpt;
	BlindingFactor offset;
	uint64_t amount;
	Fee fee;
	std::vector<SlateSignature> sigs;
	std::vector<SlateCommitment> commitments;
	std::optional<SlatePaymentProof> proofOpt;

	Slate(const uuids::uuid& uuid = uuids::uuid_system_generator()())
		: slateId{ uuid },
		stage{ ESlateStage::STANDARD_SENT },
		version{ 4 },
		blockVersion{ 5 },
		numParticipants{ 2 },
		ttl{ 0 },
		kernelFeatures{ EKernelFeatures::DEFAULT_KERNEL },
		featureArgsOpt{ std::nullopt },
		offset{},
		amount{ 0 },
		fee{},
		sigs{},
		commitments{},
		proofOpt{ std::nullopt }
	{

	}

	bool operator==(const Slate& rhs) const noexcept
	{
		return slateId == rhs.slateId &&
			stage == rhs.stage &&
			version == rhs.version &&
			blockVersion && rhs.blockVersion &&
			amount == rhs.amount &&
			fee == rhs.fee &&
			offset == rhs.offset &&
			numParticipants == rhs.numParticipants &&
			kernelFeatures == rhs.kernelFeatures &&
			ttl == rhs.ttl &&
			sigs == rhs.sigs &&
			commitments == rhs.commitments &&
			proofOpt == rhs.proofOpt;
	}

	const uuids::uuid& GetId() const noexcept { return slateId; }
	uint64_t GetAmount() const noexcept { return amount; }
	Fee GetFee() const { return fee; }
	uint64_t GetLockHeight() const noexcept { return featureArgsOpt.has_value() ? featureArgsOpt.value().lockHeightOpt.value_or(0) : 0; }
	std::optional<SlatePaymentProof>& GetPaymentProof() noexcept { return proofOpt; }
	const std::optional<SlatePaymentProof>& GetPaymentProof() const noexcept { return proofOpt; }

	std::string Format() const noexcept final { return "Slate{" + uuids::to_string(slateId) + "}"; }

	std::vector<TransactionInput> GetInputs() const noexcept;
	std::vector<TransactionOutput> GetOutputs() const noexcept;

	void AddInput(const EOutputFeatures features, const Commitment& commitment);
	void AddOutput(const EOutputFeatures features, const Commitment& commitment, const RangeProof& proof);

	PublicKey CalculateTotalExcess() const;
	PublicKey CalculateTotalNonce() const;

	void Serialize(Serializer& serializer) const;
	static Slate Deserialize(ByteBuffer& byteBuffer);

	Json::Value ToJSON() const;
	static Slate FromJSON(const Json::Value& json);
};