#include <Core/Models/FullBlock.h>

FullBlock::FullBlock(BlockHeader&& blockHeader, TransactionBody&& transactionBody)
	: m_blockHeader(std::move(blockHeader)), m_transactionBody(std::move(transactionBody)), m_validated(false)
{

}

void FullBlock::Serialize(Serializer& serializer) const
{
	m_blockHeader.Serialize(serializer);
	m_transactionBody.Serialize(serializer);
}

FullBlock FullBlock::Deserialize(ByteBuffer& byteBuffer)
{
	BlockHeader blockHeader = BlockHeader::Deserialize(byteBuffer);
	TransactionBody transactionBody = TransactionBody::Deserialize(byteBuffer);

	return FullBlock(std::move(blockHeader), std::move(transactionBody));
}