#pragma once

#include <Crypto/BulletproofType.h>
#include <Crypto/BigInteger.h>
#include <stdint.h>

class ProofMessage
{
public:
	ProofMessage(CBigInteger<20>&& proofMessageBytes)
		: m_proofMessageBytes(proofMessageBytes)
	{

	}

	static ProofMessage FromKeyIndices(const std::vector<uint32_t>& keyIndices, const EBulletproofType& bulletproofType)
	{
		Serializer serializer;
		for (const uint32_t keyIndex : keyIndices)
		{
			serializer.Append(keyIndex);
		}

		std::vector<unsigned char> paddedPath(20, 0);
		if (bulletproofType == EBulletproofType::ENHANCED)
		{
			paddedPath[2] = 1;
			paddedPath[3] = (unsigned char)keyIndices.size();
		}

		for (size_t i = 0; i < serializer.GetBytes().size(); i++)
		{
			paddedPath[i + 4] = serializer.GetBytes()[i];
		}

		return ProofMessage(CBigInteger<20>(paddedPath));
	}

	std::vector<uint32_t> ToKeyIndices(const EBulletproofType& bulletproofType) const
	{
		ByteBuffer byteBuffer(m_proofMessageBytes.GetData());

		bool switchCommits = true;
		size_t length = 3;
		if (bulletproofType == EBulletproofType::ENHANCED)
		{
			byteBuffer.ReadU8(); // RESERVED: Always 0
			byteBuffer.ReadU8(); // Wallet Type
			switchCommits = (byteBuffer.ReadU8() == 1);
			length = byteBuffer.ReadU8();
		}
		else
		{
			const uint32_t first4Bytes = byteBuffer.ReadU32();
			if (first4Bytes != 0)
			{
				throw DeserializationException();
			}
		}

		if (length == 0)
		{
			length = 3;
		}

		std::vector<uint32_t> keyIndices(length);
		for (size_t i = 0; i < length; i++)
		{
			keyIndices[i] = byteBuffer.ReadU32();
		}

		return keyIndices;
	}

	inline const CBigInteger<20>& GetBytes() const { return m_proofMessageBytes; }

private:
	CBigInteger<20> m_proofMessageBytes;
};