#include <Core/Models/TransactionKernel.h>

#include <Serialization/Serializer.h>
#include <Crypto.h>

TransactionKernel::TransactionKernel(const EKernelFeatures features, const uint64_t fee, const uint64_t lockHeight, Commitment&& excessCommitment, Signature&& excessSignature)
	: m_features(features), m_fee(fee), m_lockHeight(lockHeight), m_excessCommitment(std::move(excessCommitment)), m_excessSignature(std::move(excessSignature))
{
	Serializer serializer;
	Serialize(serializer);
	m_hash = Crypto::Blake2b(serializer.GetBytes());
}

void TransactionKernel::Serialize(Serializer& serializer) const
{
	// Serialize OutputFeatures
	serializer.Append<uint8_t>((uint8_t)m_features);

	// Serialize Fee
	serializer.Append<uint64_t>(m_fee);

	// Serialize LockHeight
	serializer.Append<uint64_t>(m_lockHeight);

	// Serialize ExcessCommitment
	m_excessCommitment.Serialize(serializer);

	// Serialize ExcessSignature
	m_excessSignature.Serialize(serializer);
}

TransactionKernel TransactionKernel::Deserialize(ByteBuffer& byteBuffer)
{
	// Read KernelFeatures (1 byte)
	const EKernelFeatures features = (EKernelFeatures)byteBuffer.ReadU8();

	// Read Fee (8 bytes)
	const uint64_t fee = byteBuffer.ReadU64();

	// Read LockHeight (8 bytes)
	const uint64_t lockHeight = byteBuffer.ReadU64();

	// Read ExcessCommitment (33 bytes)
	CBigInteger<33> commitmentBytes = byteBuffer.ReadBigInteger<33>();
	Commitment excessCommitment(std::move(commitmentBytes));

	// Read ExcessSignature (64 bytes)
	CBigInteger<64> signatureBytes = byteBuffer.ReadBigInteger<64>();
	Signature excessSignature(std::move(signatureBytes));

	return TransactionKernel((EKernelFeatures)features, fee, lockHeight, std::move(excessCommitment), std::move(excessSignature));
}

Hash TransactionKernel::GetSignatureMessage() const
{
	Serializer serializer;
	serializer.Append<uint8_t>((uint8_t)GetFeatures());

	if (GetFeatures() != EKernelFeatures::COINBASE_KERNEL)
	{
		serializer.Append<uint64_t>(GetFee());
	}

	if (GetFeatures() == EKernelFeatures::HEIGHT_LOCKED)
	{
		serializer.Append<uint64_t>(GetLockHeight());
	}

	const std::vector<unsigned char>& message = serializer.GetBytes();

	return Crypto::Blake2b(message);
}

const Hash& TransactionKernel::GetHash() const
{
	return m_hash;
}