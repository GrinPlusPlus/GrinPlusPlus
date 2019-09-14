#include <Core/Models/ProofOfWork.h>
#include <Consensus/BlockDifficulty.h>
#include <Crypto/Crypto.h>

ProofOfWork::ProofOfWork(const uint8_t edgeBits, std::vector<uint64_t>&& proofNonces)
	: m_edgeBits(edgeBits),
	m_proofNonces(std::move(proofNonces))
{

}

ProofOfWork::ProofOfWork(const uint8_t edgeBits, std::vector<uint64_t>&& proofNonces, Hash&& hash)
	: m_edgeBits(edgeBits),
	m_proofNonces(std::move(proofNonces)),
	m_hash(hash)
{

}

void ProofOfWork::Serialize(Serializer& serializer) const
{
	serializer.Append<uint8_t>(m_edgeBits);
	SerializeProofNonces(serializer);
}

// TODO: Rename "SerializeCycle"
void ProofOfWork::SerializeProofNonces(Serializer& serializer) const
{
	const int bytes_len = ((m_edgeBits * Consensus::PROOFSIZE) + 7) / 8;
	std::vector<unsigned char> bits(bytes_len, 0);
	for (int n = 0; n < m_proofNonces.size(); n++)
	{
		for (int bit = 0; bit < m_edgeBits; bit++)
		{
			const uint64_t nonce = m_proofNonces[n];
			if ((nonce & ((uint64_t)1 << bit)) != 0)
			{
				const size_t positionTemp = (n * m_edgeBits) + bit;

				bits[positionTemp / 8] |= (uint8_t)(1 << (positionTemp % 8));
			}
		}
	}

	serializer.AppendByteVector(bits);
}

ProofOfWork ProofOfWork::Deserialize(ByteBuffer& byteBuffer)
{
	const uint8_t edgeBits = byteBuffer.ReadU8();

	const int bytes_len = ((edgeBits * Consensus::PROOFSIZE) + 7) / 8;
	const std::vector<unsigned char> bits = byteBuffer.ReadVector(bytes_len);
	Hash hash = Crypto::Blake2b(bits);

	std::vector<uint64_t> proofNonces = DeserializeProofNonces(bits, edgeBits);

	return ProofOfWork(edgeBits, std::move(proofNonces), std::move(hash));
}

std::vector<uint64_t> ProofOfWork::DeserializeProofNonces(const std::vector<unsigned char>& bits, const uint8_t edgeBits)
{
	std::vector<uint64_t> proofNonces;
	for (int n = 0; n < Consensus::PROOFSIZE; n++)
	{
		uint64_t proofNonce = 0;
		for (int bit = 0; bit < edgeBits; bit++)
		{
			const size_t positionTemp = (n * edgeBits) + bit;

			if ((bits[positionTemp / 8] & (1 << ((uint8_t)positionTemp % 8))) != 0)
			{
				proofNonce |= ((uint64_t)1 << bit);
			}
		}

		proofNonces.push_back(proofNonce);
	}

	return proofNonces;
}

const CBigInteger<32>& ProofOfWork::GetHash() const
{
	if (m_hash == CBigInteger<32>())
	{
		Serializer serializer;
		SerializeProofNonces(serializer);
		m_hash = Crypto::Blake2b(serializer.GetBytes());
	}

	return m_hash;
}