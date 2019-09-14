#include <Core/Models/ShortId.h>
#include <Crypto/Crypto.h>

ShortId::ShortId(CBigInteger<6>&& id)
	: m_id(id)
{

}

ShortId ShortId::Create(const CBigInteger<32>& hash, const CBigInteger<32>& blockHash, const uint64_t nonce)
{
	// take the block hash and the nonce and hash them together
	Serializer serializer;
	serializer.AppendBigInteger<32>(blockHash);
	serializer.Append<uint64_t>(nonce);
	const CBigInteger<32> hashWithNonce = Crypto::Blake2b(serializer.GetBytes());

	// extract k0/k1 from the block_hash
	ByteBuffer byteBuffer(hashWithNonce.GetData());
	const uint64_t k0 = byteBuffer.ReadU64_LE();
	const uint64_t k1 = byteBuffer.ReadU64_LE();

	// SipHash24 our hash using the k0 and k1 keys
	const uint64_t sipHash = Crypto::SipHash24(k0, k1, hash.GetData());

	// construct a short_id from the resulting bytes (dropping the 2 most significant bytes)
	Serializer serializer2;
	serializer2.AppendLittleEndian<uint64_t>(sipHash);

	return ShortId(CBigInteger<6>(&serializer2.GetBytes()[0]));
}

void ShortId::Serialize(Serializer& serializer) const
{
	serializer.AppendBigInteger<6>(m_id);
}

ShortId ShortId::Deserialize(ByteBuffer& byteBuffer)
{
	CBigInteger<6> id = byteBuffer.ReadVector(6);

	return ShortId(std::move(id));
}

Hash ShortId::GetHash() const
{
	return Crypto::Blake2b(m_id.GetData());
}