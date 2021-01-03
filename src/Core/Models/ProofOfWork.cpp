#include <Core/Models/ProofOfWork.h>
#include <Consensus.h>
#include <Crypto/Hasher.h>

ProofOfWork::ProofOfWork(const uint8_t edgeBits, std::vector<uint64_t>&& proofNonces)
	: m_edgeBits(edgeBits),
	m_proofNonces(std::move(proofNonces))
{
	Serializer serializer;
	SerializeCycle(serializer);
	m_hash = Hasher::Blake2b(serializer.GetBytes());
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
	SerializeCycle(serializer);
}

void ProofOfWork::SerializeCycle(Serializer& serializer) const
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
	Hash hash = Hasher::Blake2b(bits);

	std::vector<uint64_t> proofNonces = DeserializeProofNonces(bits, edgeBits);

	return ProofOfWork(edgeBits, std::move(proofNonces), std::move(hash));
}

std::vector<uint64_t> ProofOfWork::DeserializeProofNonces(const std::vector<unsigned char>& bits, const uint8_t edgeBits)
{
	if (edgeBits == 0 || edgeBits > 63)
	{
		throw DESERIALIZATION_EXCEPTION_F("Invalid number of edge bits: {}", edgeBits);
	}

	//const uint64_t nonce_bits = (uint64_t)edgeBits;
	//const uint64_t bits_len = nonce_bits * Consensus::PROOFSIZE;
	//const uint64_t bytes_len = (bits_len + 7) / 8;

	std::vector<uint64_t> proofNonces;
	for (int n = 0; n < Consensus::PROOFSIZE; n++)
	{
		//const uint64_t bit_start = ((uint64_t)n * (uint64_t)edgeBits);

		//uint64_t read_from = bit_start / 8;
		//if ((read_from + 8) > bits.size())
		//{
		//	read_from = bits.size() - 8;
		//}

		//const uint64_t max_bit_end = (read_from + 8) * 8;
		//const uint64_t max_pos = bit_start + (uint64_t)edgeBits;

		//if (max_pos <= max_bit_end)
		//{

		//}
		//else
		//{

		//}

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

const Hash& ProofOfWork::GetHash() const
{
	return m_hash;
}