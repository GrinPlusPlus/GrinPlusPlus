#include <Wallet/Models/Slate/Slate.h>
#include <Common/Util/BitUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Crypto/Crypto.h>

// Bit     4      3      2      1      0
// field  ttl   feat    fee    Amt  num_parts
struct OptionalFieldStatus
{
	bool include_num_parts;
	bool include_amt;
	bool include_fee;
	bool include_feat;
	bool include_ttl;

	uint8_t ToByte() const noexcept
	{
		uint8_t status = 0;
		status |= include_num_parts ? 1 : 0;
		status |= include_amt ? 1 << 1 : 0;
		status |= include_fee ? 1 << 2 : 0;
		status |= include_feat ? 1 << 3 : 0;
		status |= include_ttl ? 1 << 4 : 0;
		return status;
	}

	static OptionalFieldStatus FromByte(const uint8_t byte) noexcept
	{
		OptionalFieldStatus status;
		status.include_num_parts = BitUtil::IsSet(byte, 0);
		status.include_amt = BitUtil::IsSet(byte, 1);
		status.include_fee = BitUtil::IsSet(byte, 2);
		status.include_feat = BitUtil::IsSet(byte, 3);
		status.include_ttl = BitUtil::IsSet(byte, 4);
		return status;
	}
};

// Bit      1     0
// field  proof  coms
struct OptionalStructStatus
{
	bool include_coms;
	bool include_proof;

	uint8_t ToByte() const noexcept
	{
		uint8_t status = 0;
		status |= include_coms ? 1 : 0;
		status |= include_proof ? 1 << 1 : 0;
		return status;
	}

	static OptionalStructStatus FromByte(const uint8_t byte) noexcept
	{
		OptionalStructStatus status;
		status.include_coms = BitUtil::IsSet(byte, 0);
		status.include_proof = BitUtil::IsSet(byte, 1);
		return status;
	}
};

std::vector<TransactionInput> Slate::GetInputs() const noexcept
{
	std::vector<TransactionInput> inputs;
	for (const SlateCommitment& com : commitments)
	{
		if (!com.proofOpt.has_value()) {
			inputs.push_back(TransactionInput{ com.commitment });
		}
	}

	return inputs;
}

std::vector<TransactionOutput> Slate::GetOutputs() const noexcept
{
	std::vector<TransactionOutput> outputs;
	for (const SlateCommitment& com : commitments)
	{
		if (com.proofOpt.has_value()) {
			outputs.push_back(TransactionOutput{ com.features, com.commitment, com.proofOpt.value() });
		}
	}

	return outputs;
}

void Slate::AddInput(const EOutputFeatures features, const Commitment& commitment)
{
	for (const auto& com : commitments)
	{
		if (com.commitment == commitment) {
			return;
		}
	}

	commitments.push_back(SlateCommitment{ features, commitment, std::nullopt });
}

void Slate::AddOutput(const EOutputFeatures features, const Commitment& commitment, const RangeProof& proof)
{
	for (auto& com : commitments)
	{
		if (com.commitment == commitment) {
			if (!com.proofOpt.has_value()) {
				com.proofOpt = std::make_optional(RangeProof(proof));
			}

			return;
		}
	}

	commitments.push_back(SlateCommitment{ features, commitment, std::make_optional(RangeProof(proof)) });
}

PublicKey Slate::CalculateTotalExcess() const
{
	std::vector<PublicKey> public_keys;
	std::transform(
		sigs.cbegin(), sigs.cend(),
		std::back_inserter(public_keys),
		[](const SlateSignature& signature) {
			WALLET_INFO_F("Adding Pubkey: {}", signature.excess);
			return signature.excess;
		}
	);

	PublicKey total_excess = Crypto::AddPublicKeys(public_keys);
	WALLET_INFO_F("Total excess: {}", total_excess);
	return total_excess;
}

PublicKey Slate::CalculateTotalNonce() const
{
	std::vector<PublicKey> public_nonces;
	std::transform(
		sigs.cbegin(), sigs.cend(),
		std::back_inserter(public_nonces),
		[](const SlateSignature& signature) {
			WALLET_INFO_F("Adding Pubkey: {}", signature.nonce);
			return signature.nonce;
		}
	);

	PublicKey total_nonce = Crypto::AddPublicKeys(public_nonces);
	WALLET_INFO_F("Total nonce: {}", total_nonce);
	return total_nonce;
}

/*
	ver.slate_version	u16	2
	ver.block_header_version	u16	2
	id	Uuid	16	binary Uuid representation
	sta	u8	1	See Status Byte
	offset	BlindingFactor	32
	Optional field status	u8	1	See Optional Field Status
	num_parts	u8	(1)	If present
	amt	u64	(4)	If present
	fee	u64	(4)	If present
	feat	u8	(1)	If present
	ttl	u64	(4)	If present
	sigs length	u8	1	Number of entries in the sigs struct
	sigs entries	struct	varies	See Sigs Entries
	Optional struct status	u8	1	See Optional Struct Status
	coms length	u16	(2)	If present
	coms entries	struct	(varies)	If present. See Coms Entries
	proof	struct	(varies)	If present. See Proof
	feat_args entries	struct	(varies)	If present. See Feature Args
*/
void Slate::Serialize(Serializer& serializer) const
{
	serializer.Append<uint16_t>(version);
	serializer.Append<uint16_t>(blockVersion);
	serializer.AppendBigInteger<16>(CBigInteger<16>{ (uint8_t*)slateId.as_bytes().data() });
	serializer.Append<uint8_t>(stage.ToByte());
	serializer.AppendBigInteger<32>(offset.GetBytes());

	OptionalFieldStatus fieldStatus;
	fieldStatus.include_num_parts = (numParticipants != 2);
	fieldStatus.include_amt = (amount != 0);
	fieldStatus.include_fee = (fee != Fee());
	fieldStatus.include_feat = (kernelFeatures != EKernelFeatures::DEFAULT_KERNEL);
	fieldStatus.include_ttl = (ttl != 0);
	serializer.Append<uint8_t>(fieldStatus.ToByte());

	if (fieldStatus.include_num_parts) {
		serializer.Append<uint8_t>(numParticipants);
	}

	if (fieldStatus.include_amt) {
		serializer.Append<uint64_t>(amount);
	}

	if (fieldStatus.include_fee) {
		fee.Serialize(serializer);
	}

	if (fieldStatus.include_feat) {
		serializer.Append<uint8_t>((uint8_t)kernelFeatures);
	}

	if (fieldStatus.include_ttl) {
		serializer.Append<uint64_t>(ttl);
	}

	serializer.Append<uint8_t>((uint8_t)sigs.size());
	for (const auto& sig : sigs)
	{
		sig.Serialize(serializer);
	}

	OptionalStructStatus structStatus;
	structStatus.include_coms = !commitments.empty();
	structStatus.include_proof = proofOpt.has_value();
	serializer.Append<uint8_t>(structStatus.ToByte());

	if (structStatus.include_coms) {
		serializer.Append<uint16_t>((uint16_t)commitments.size());

		for (const auto& commitment : commitments)
		{
			commitment.Serialize(serializer);
		}
	}

	if (structStatus.include_proof) {
		proofOpt.value().Serialize(serializer);
	}

	if (kernelFeatures == EKernelFeatures::HEIGHT_LOCKED) {
		serializer.Append<uint64_t>(GetLockHeight());
	}
}

Slate Slate::Deserialize(ByteBuffer& byteBuffer)
{
	Slate slate;
	slate.version = byteBuffer.ReadU16();
	slate.blockVersion = byteBuffer.ReadU16();

	std::vector<unsigned char> slateIdVec = byteBuffer.ReadBigInteger<16>().GetData();
	std::array<unsigned char, 16> slateIdArr;
	std::copy_n(slateIdVec.begin(), 16, slateIdArr.begin());
	slate.slateId = uuids::uuid(slateIdArr);

	slate.stage = SlateStage::FromByte(byteBuffer.ReadU8());
	slate.offset = byteBuffer.ReadBigInteger<32>();

	OptionalFieldStatus fieldStatus = OptionalFieldStatus::FromByte(byteBuffer.ReadU8());

	if (fieldStatus.include_num_parts) {
		slate.numParticipants = byteBuffer.ReadU8();
	}

	if (fieldStatus.include_amt) {
		slate.amount = byteBuffer.ReadU64();
	}

	if (fieldStatus.include_fee) {
		slate.fee = Fee::Deserialize(byteBuffer);
	}

	if (fieldStatus.include_feat) {
		slate.kernelFeatures = (EKernelFeatures)byteBuffer.ReadU8();
	}

	if (fieldStatus.include_ttl) {
		slate.ttl = byteBuffer.ReadU64();
	}

	const uint8_t numSigs = byteBuffer.ReadU8();
	for (uint8_t s = 0; s < numSigs; s++)
	{
		slate.sigs.push_back(SlateSignature::Deserialize(byteBuffer));
	}

	OptionalStructStatus structStatus = OptionalStructStatus::FromByte(byteBuffer.ReadU8());

	if (structStatus.include_coms) {
		const uint16_t numComs = byteBuffer.ReadU16();

		for (uint16_t c = 0; c < numComs; c++)
		{
			slate.commitments.push_back(SlateCommitment::Deserialize(byteBuffer));
		}
	}

	if (structStatus.include_proof) {
		slate.proofOpt = std::make_optional<SlatePaymentProof>(SlatePaymentProof::Deserialize(byteBuffer));
	}

	if (slate.kernelFeatures != EKernelFeatures::DEFAULT_KERNEL) {
		SlateFeatureArgs featureArgs;
		if (slate.kernelFeatures == EKernelFeatures::HEIGHT_LOCKED) {
			featureArgs.lockHeightOpt = std::make_optional<uint64_t>(byteBuffer.ReadU64());
		}
		slate.featureArgsOpt = std::make_optional<SlateFeatureArgs>(std::move(featureArgs));
	}

	return slate;
}

Json::Value Slate::ToJSON() const
{
	Json::Value json;
	json["ver"] = StringUtil::Format("{}:{}", version, blockVersion);
	json["id"] = uuids::to_string(slateId);
	json["sta"] = stage.ToString();

	if (!offset.IsNull()) {
		json["off"] = offset.ToHex();
	}

	if (numParticipants != 2) {
		json["num_parts"] = std::to_string(numParticipants);
	}

	if (fee != Fee{}) {
		json["fee"] = fee.ToJSON();
	}

	if (amount != 0) {
		json["amt"] = std::to_string(amount);
	}

	if (kernelFeatures != EKernelFeatures::DEFAULT_KERNEL) {
		json["feat"] = (uint8_t)kernelFeatures;
		assert(featureArgsOpt.has_value());
		json["feat_args"] = featureArgsOpt.value().ToJSON();
	}

	if (ttl != 0) {
		json["ttl"] = std::to_string(ttl);
	}

	Json::Value sigsJson(Json::arrayValue);
	for (const auto& sig : sigs)
	{
		sigsJson.append(sig.ToJSON());
	}
	json["sigs"] = sigsJson;

	Json::Value comsJson(Json::arrayValue);
	for (const auto& commitment : commitments)
	{
		comsJson.append(commitment.ToJSON());
	}
	json["coms"] = comsJson;

	if (proofOpt.has_value()) {
		json["proof"] = proofOpt.value().ToJSON();
	}

	return json;
}

Slate Slate::FromJSON(const Json::Value& json)
{
	std::vector<std::string> versionParts = StringUtil::Split(JsonUtil::GetRequiredString(json, "ver"), ":");
	if (versionParts.size() != 2)
	{
		throw std::exception();
	}

	std::optional<uuids::uuid> slateIdOpt = uuids::uuid::from_string(JsonUtil::GetRequiredString(json, "id"));
	if (!slateIdOpt.has_value())
	{
		throw DESERIALIZE_FIELD_EXCEPTION("id");
	}

	auto featuresOpt = JsonUtil::GetOptionalField(json, "feat");

	Slate slate;
	slate.version = (uint16_t)std::stoul(versionParts[0]);
	slate.blockVersion = (uint16_t)std::stoul(versionParts[1]);
	slate.slateId = slateIdOpt.value();
	slate.stage = SlateStage::FromString(JsonUtil::GetRequiredString(json, "sta"));
	slate.offset = JsonUtil::GetBlindingFactorOpt(json, "off").value_or(BlindingFactor{});

	slate.numParticipants = JsonUtil::GetUInt8Opt(json, "num_parts").value_or(2);

	auto fee_json_opt = JsonUtil::GetOptionalField(json, "fee");
	if (fee_json_opt.has_value()) {
		slate.fee = Fee::FromJSON(fee_json_opt.value());
	}

	slate.amount = JsonUtil::GetUInt64Opt(json, "amt").value_or(0);
	slate.kernelFeatures = (EKernelFeatures)JsonUtil::GetUInt8Opt(json, "feat").value_or(0);
	// TODO: Feat args
	slate.ttl = JsonUtil::GetUInt64Opt(json, "ttl").value_or(0);

	std::vector<Json::Value> sigs = JsonUtil::GetRequiredArray(json, "sigs");
	for (const Json::Value& sig : sigs)
	{
		slate.sigs.push_back(SlateSignature::FromJSON(sig));
	}

	std::vector<Json::Value> commitmentsJson = JsonUtil::GetArray(json, "coms");
	for (const Json::Value& commitmentJson : commitmentsJson)
	{
		slate.commitments.push_back(SlateCommitment::FromJSON(commitmentJson));
	}

	std::optional<Json::Value> proofJsonOpt = JsonUtil::GetOptionalField(json, "proof");
	if (proofJsonOpt.has_value() && !proofJsonOpt.value().isNull()) {
		slate.proofOpt = std::make_optional(SlatePaymentProof::FromJSON(proofJsonOpt.value()));
	}

	return slate;
}