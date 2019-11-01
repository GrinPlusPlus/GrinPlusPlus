#include "KernelMMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Core/Traits/Lockable.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

KernelMMR::KernelMMR(std::shared_ptr<HashFile> pHashFile, std::shared_ptr<DataFile<KERNEL_SIZE>> pDataFile)
	: m_pHashFile(pHashFile),
	m_pDataFile(pDataFile)
{

}

KernelMMR::~KernelMMR()
{
}

std::shared_ptr<KernelMMR> KernelMMR::Load(const std::string& txHashSetDirectory)
{
	std::shared_ptr<HashFile> pHashFile = HashFile::Load(txHashSetDirectory + "kernel/pmmr_hash.bin");
	std::shared_ptr<DataFile<KERNEL_SIZE>> pDataFile = DataFile<KERNEL_SIZE>::Load(txHashSetDirectory + "kernel/pmmr_data.bin");

	return std::make_shared<KernelMMR>(KernelMMR(pHashFile, pDataFile));
}

Hash KernelMMR::Root(const uint64_t size) const
{
	return MMRHashUtil::Root(m_pHashFile, size, nullptr);
}

std::unique_ptr<TransactionKernel> KernelMMR::GetKernelAt(const uint64_t mmrIndex) const
{
	if (MMRUtil::IsLeaf(mmrIndex))
	{
		const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);

		std::vector<unsigned char> data = m_pDataFile->GetDataAt(numLeaves - 1);

		if (data.size() == KERNEL_SIZE)
		{
			ByteBuffer byteBuffer(data);
			return std::make_unique<TransactionKernel>(TransactionKernel::Deserialize(byteBuffer));
		}
	}

	return std::unique_ptr<TransactionKernel>(nullptr);
}

std::vector<Hash> KernelMMR::GetLastLeafHashes(const uint64_t numHashes) const
{
	return MMRHashUtil::GetLastLeafHashes(m_pHashFile, nullptr, nullptr, numHashes);
}

bool KernelMMR::Rewind(const uint64_t size)
{
	m_pHashFile->Rewind(size);
	m_pHashFile->Rewind(MMRUtil::GetNumLeaves(size - 1));
	return true;
}

void KernelMMR::Commit()
{
	LOG_TRACE_F("Flushing with size (%llu)", GetSize());
	m_pHashFile->Commit();
	m_pHashFile->Commit();
}

void KernelMMR::Rollback()
{
	LOG_DEBUG("Discarding changes since last flush");
	m_pHashFile->Rollback();
	m_pDataFile->Rollback();
}

bool KernelMMR::ApplyKernel(const TransactionKernel& kernel)
{
	// Add to data file
	Serializer serializer;
	kernel.Serialize(serializer);
	m_pDataFile->AddData(serializer.GetBytes());

	// Add hashes
	MMRHashUtil::AddHashes(m_pHashFile, serializer.GetBytes(), nullptr);

	return true;
}