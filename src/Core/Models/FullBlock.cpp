#include <Core/Models/FullBlock.h>

FullBlock::FullBlock(BlockHeaderPtr pBlockHeader, TransactionBody&& transactionBody)
	: m_pBlockHeader(pBlockHeader), m_transactionBody(std::move(transactionBody)), m_validated(false)
{

}

void FullBlock::Serialize(Serializer& serializer) const
{
	m_pBlockHeader->Serialize(serializer);
	m_transactionBody.Serialize(serializer);
}

FullBlock FullBlock::Deserialize(ByteBuffer& byteBuffer)
{
	BlockHeaderPtr pBlockHeader = std::make_shared<const BlockHeader>(BlockHeader::Deserialize(byteBuffer));
	TransactionBody transactionBody = TransactionBody::Deserialize(byteBuffer);

	return FullBlock(pBlockHeader, std::move(transactionBody));
}