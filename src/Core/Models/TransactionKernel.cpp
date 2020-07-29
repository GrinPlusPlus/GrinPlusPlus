#include <Core/Models/TransactionKernel.h>

#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <Crypto/Crypto.h>
#include <Consensus/BlockTime.h>

TransactionKernel::TransactionKernel(const EKernelFeatures features, const uint64_t fee, const uint64_t lockHeight, Commitment&& excessCommitment, Signature&& excessSignature)
	: m_features(features), m_fee(fee), m_lockHeight(lockHeight), m_excessCommitment(std::move(excessCommitment)), m_excessSignature(std::move(excessSignature))
{
}

TransactionKernel::TransactionKernel(const EKernelFeatures features, const uint64_t fee, const uint64_t lockHeight, const Commitment& excessCom, const Signature& excessSig)
	: m_features(features), m_fee(fee), m_lockHeight(lockHeight), m_excessCommitment(excessCom), m_excessSignature(excessSig)
{
}

void TransactionKernel::Serialize(Serializer& serializer) const
{
	if (serializer.GetProtocolVersion() == EProtocolVersion::V2)
	{
		serializer.Append<uint8_t>((uint8_t)GetFeatures());
		switch (GetFeatures())
		{
			case EKernelFeatures::DEFAULT_KERNEL:
				serializer.Append<uint64_t>(GetFee());
				break;
			case EKernelFeatures::COINBASE_KERNEL:
				break;
			case EKernelFeatures::HEIGHT_LOCKED:
				serializer.Append<uint64_t>(GetFee());
				serializer.Append<uint64_t>(GetLockHeight());
				break;
			case EKernelFeatures::NO_RECENT_DUPLICATE:
				serializer.Append<uint64_t>(GetFee());
				serializer.Append<uint16_t>((uint16_t)GetLockHeight());
				break;
		}

		m_excessCommitment.Serialize(serializer);
		m_excessSignature.Serialize(serializer);
	}
	else
	{
		serializer.Append<uint8_t>((uint8_t)m_features);
		serializer.Append<uint64_t>(m_fee);
		serializer.Append<uint64_t>(m_lockHeight);
		m_excessCommitment.Serialize(serializer);
		m_excessSignature.Serialize(serializer);
	}
}

TransactionKernel TransactionKernel::Deserialize(ByteBuffer& byteBuffer)
{
	// Read KernelFeatures (1 byte)
	const EKernelFeatures features = (EKernelFeatures)byteBuffer.ReadU8();

	uint64_t fee = 0;
	uint64_t lockHeight = 0;
	if (byteBuffer.GetProtocolVersion() == EProtocolVersion::V2)
	{
		if (features != EKernelFeatures::COINBASE_KERNEL)
		{
			fee = byteBuffer.ReadU64();
		}

		if (features == EKernelFeatures::HEIGHT_LOCKED)
		{
			lockHeight = byteBuffer.ReadU64();
		}
		else if (features == EKernelFeatures::NO_RECENT_DUPLICATE)
		{
			lockHeight = byteBuffer.ReadU16();
		}
	}
	else
	{
		fee = byteBuffer.ReadU64();
		lockHeight = byteBuffer.ReadU64();
	}

	// Read ExcessCommitment (33 bytes)
	CBigInteger<33> commitmentBytes = byteBuffer.ReadBigInteger<33>();
	Commitment excessCommitment(std::move(commitmentBytes));

	// Read ExcessSignature (64 bytes)
	CBigInteger<64> signatureBytes = byteBuffer.ReadBigInteger<64>();
	Signature excessSignature(std::move(signatureBytes));

	if (features == EKernelFeatures::NO_RECENT_DUPLICATE)
	{
		if (lockHeight == 0 || lockHeight > Consensus::WEEK_HEIGHT)
		{
			throw DESERIALIZATION_EXCEPTION_F("Invalid NRD relative height({}) for kernel: {}", lockHeight, excessCommitment);
		}
	}

	return TransactionKernel((EKernelFeatures)features, fee, lockHeight, std::move(excessCommitment), std::move(excessSignature));
}

Json::Value TransactionKernel::ToJSON() const
{
	Json::Value kernelNode;
	Json::Value features_json;
	Json::Value features_attrs_json;

	if (m_features != EKernelFeatures::COINBASE_KERNEL) {
		features_attrs_json["fee"] = GetFee();
	}

	if (m_features == EKernelFeatures::HEIGHT_LOCKED) {
		features_attrs_json["lock_height"] = GetLockHeight();
	}

	features_json[KernelFeatures::ToString(GetFeatures())] = features_attrs_json;

	kernelNode["features"] = features_json;
	kernelNode["excess"] = JsonUtil::ConvertToJSON(GetExcessCommitment());
	kernelNode["excess_sig"] = Crypto::ToCompact(GetExcessSignature()).ToHex();
	return kernelNode;
}

TransactionKernel TransactionKernel::FromJSON(const Json::Value& transactionKernelJSON)
{
	Json::Value features_json = JsonUtil::GetRequiredField(transactionKernelJSON, "features");
	std::string feature_str = features_json.getMemberNames().front();
	EKernelFeatures features = KernelFeatures::FromString(feature_str);

	Json::Value feature_json = features_json.get(feature_str, Json::nullValue);

	uint64_t fee = JsonUtil::GetUInt64Opt(feature_json, "fee").value_or(0);
	uint64_t lockHeight = JsonUtil::GetUInt64Opt(feature_json, "lock_height").value_or(0);

	Commitment excessCommitment = JsonUtil::GetCommitment(transactionKernelJSON, "excess");
	Signature excessSignature = Crypto::ParseCompactSignature(JsonUtil::GetCompactSignature(transactionKernelJSON, "excess_sig"));

	if (features == EKernelFeatures::NO_RECENT_DUPLICATE)
	{
		if (lockHeight == 0 || lockHeight > Consensus::WEEK_HEIGHT)
		{
			throw DESERIALIZATION_EXCEPTION_F("Invalid NRD relative height({}) for kernel: {}", lockHeight, excessCommitment);
		}
	}

	return TransactionKernel(features, fee, lockHeight, std::move(excessCommitment), std::move(excessSignature));
}

Hash TransactionKernel::GetSignatureMessage(const EKernelFeatures features, const uint64_t fee, const uint64_t lockHeight)
{
	Serializer serializer;
	serializer.Append<uint8_t>((uint8_t)features);

	if (features != EKernelFeatures::COINBASE_KERNEL)
	{
		serializer.Append<uint64_t>(fee);
	}

	if (features == EKernelFeatures::HEIGHT_LOCKED)
	{
		serializer.Append<uint64_t>(lockHeight);
	}

	if (features == EKernelFeatures::NO_RECENT_DUPLICATE)
	{
		serializer.Append<uint16_t>((uint16_t)lockHeight);
	}

	return Crypto::Blake2b(serializer.GetBytes());
}

Hash TransactionKernel::GetSignatureMessage() const
{
	return GetSignatureMessage(m_features, m_fee, m_lockHeight);
}

const Hash& TransactionKernel::GetHash() const
{
	if (m_hash == Hash{}) {
		Serializer serializer{ EProtocolVersion::V1 };
		Serialize(serializer);
		m_hash = Crypto::Blake2b(serializer.GetBytes());
	}

	return m_hash;
}