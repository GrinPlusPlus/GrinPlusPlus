#include <Core/Models/TransactionKernel.h>

#include <Core/Serialization/Serializer.h>
#include <Core/Util/JsonUtil.h>
#include <Crypto/Crypto.h>
#include <Consensus/BlockTime.h>

TransactionKernel::TransactionKernel(const EKernelFeatures features, const uint64_t fee, const uint64_t lockHeight, Commitment&& excessCommitment, Signature&& excessSignature)
	: m_features(features), m_fee(fee), m_lockHeight(lockHeight), m_excessCommitment(std::move(excessCommitment)), m_excessSignature(std::move(excessSignature))
{
	Serializer serializer;
	Serialize(serializer);
	m_hash = Crypto::Blake2b(serializer.GetBytes());
}

TransactionKernel::TransactionKernel(const EKernelFeatures features, const uint64_t fee, const uint64_t lockHeight, const Commitment& excessCom, const Signature& excessSig)
	: m_features(features), m_fee(fee), m_lockHeight(lockHeight), m_excessCommitment(excessCom), m_excessSignature(excessSig)
{
	Serializer serializer;
	Serialize(serializer);
	m_hash = Crypto::Blake2b(serializer.GetBytes());
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
	kernelNode["features"] = KernelFeatures::ToString(GetFeatures());
	kernelNode["fee"] = std::to_string(GetFee());
	kernelNode["lock_height"] = std::to_string(GetLockHeight());
	kernelNode["excess"] = JsonUtil::ConvertToJSON(GetExcessCommitment());
	kernelNode["excess_sig"] = Crypto::ToCompact(GetExcessSignature()).ToHex();
	return kernelNode;
}

TransactionKernel TransactionKernel::FromJSON(const Json::Value& transactionKernelJSON)
{
	const EKernelFeatures features = KernelFeatures::FromString(JsonUtil::GetRequiredString(transactionKernelJSON, "features"));
	const uint64_t fee = JsonUtil::GetRequiredUInt64(transactionKernelJSON, "fee");
	const uint64_t lockHeight = JsonUtil::GetRequiredUInt64(transactionKernelJSON, "lock_height");
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
	return m_hash;
}