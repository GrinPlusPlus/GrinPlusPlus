#pragma once

#include <Crypto/BulletproofType.h>
#include <Crypto/Models/BigInteger.h>
#include <cstdint>

class ProofMessage
{
public:
	ProofMessage() = default;
	ProofMessage(CBigInteger<20>&& proofMessageBytes) : m_proofMessageBytes(std::move(proofMessageBytes)) { }

	static ProofMessage FromKeyIndices(const std::vector<uint32_t>& keyIndices, const EBulletproofType& bulletproofType)
	{
		Serializer serializer;
		for (const uint32_t keyIndex : keyIndices)
		{
			serializer.Append(keyIndex);
		}

		CBigInteger<20> paddedPath;
		if (bulletproofType == EBulletproofType::ENHANCED) {
			paddedPath[2] = 1;
			paddedPath[3] = (unsigned char)keyIndices.size();
		}

		for (size_t i = 0; i < serializer.GetBytes().size(); i++)
		{
			paddedPath[i + 4] = serializer.GetBytes()[i];
		}

		return ProofMessage(std::move(paddedPath));
	}

	std::vector<uint32_t> ToKeyIndices(const EBulletproofType& bulletproofType) const
	{
		ByteBuffer byteBuffer(m_proofMessageBytes.GetData());

		size_t length = 3;
		if (bulletproofType == EBulletproofType::ENHANCED) {
			byteBuffer.ReadU8(); // RESERVED: Always 0
			byteBuffer.ReadU8(); // Wallet Type
			byteBuffer.ReadU8(); // Switch Commits - Always true for now.
			length = byteBuffer.ReadU8();
		} else {
			const uint32_t first4Bytes = byteBuffer.ReadU32();
			if (first4Bytes != 0) {
				throw DESERIALIZATION_EXCEPTION("Expected first 4 bytes to be 0");
			}
		}

		if (length == 0) {
			length = 3;
		}

		std::vector<uint32_t> keyIndices(length);
		for (size_t i = 0; i < length; i++)
		{
			keyIndices[i] = byteBuffer.ReadU32();
		}

		return keyIndices;
	}

	const CBigInteger<20>& GetBytes() const noexcept { return m_proofMessageBytes; }
	unsigned char* data() noexcept { return m_proofMessageBytes.data(); }
	const unsigned char* data() const noexcept { return m_proofMessageBytes.data(); }

private:
	CBigInteger<20> m_proofMessageBytes;
};