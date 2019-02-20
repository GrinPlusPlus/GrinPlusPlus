#include "KernelMMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

KernelMMR::KernelMMR(HashFile* pHashFile, DataFile<KERNEL_SIZE>* pDataFile)
	: m_pHashFile(pHashFile),
	m_pDataFile(pDataFile)
{

}

KernelMMR::~KernelMMR()
{
	delete m_pHashFile;
	m_pHashFile = nullptr;

	delete m_pDataFile;
	m_pDataFile = nullptr;
}

KernelMMR* KernelMMR::Load(const std::string& txHashSetDirectory)
{
	HashFile* pHashFile = new HashFile(txHashSetDirectory + "kernel/pmmr_hash.bin");
	pHashFile->Load();

	DataFile<KERNEL_SIZE>* pDataFile = new DataFile<KERNEL_SIZE>(txHashSetDirectory + "kernel/pmmr_data.bin");
	pDataFile->Load();

	return new KernelMMR(pHashFile, pDataFile);
}

Hash KernelMMR::Root(const uint64_t size) const
{
	return MMRHashUtil::Root(*m_pHashFile, size, nullptr);
}

std::unique_ptr<TransactionKernel> KernelMMR::GetKernelAt(const uint64_t mmrIndex) const
{
	if (MMRUtil::IsLeaf(mmrIndex))
	{
		const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);

		std::vector<unsigned char> data;
		m_pDataFile->GetDataAt(numLeaves - 1, data);

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
	return MMRHashUtil::GetLastLeafHashes(*m_pHashFile, nullptr, nullptr, numHashes);
}

bool KernelMMR::Rewind(const uint64_t size)
{
	const bool hashRewind = m_pHashFile->Rewind(size);
	const bool dataRewind = m_pDataFile->Rewind(MMRUtil::GetNumLeaves(size - 1));

	return hashRewind && dataRewind;
}

bool KernelMMR::Flush()
{
	LoggerAPI::LogTrace(StringUtil::Format("KernelMMR::Flush - Flushing with size (%llu)", GetSize()));
	const bool hashFlush = m_pHashFile->Flush();
	const bool dataFlush = m_pDataFile->Flush();

	return hashFlush && dataFlush;
}

bool KernelMMR::Discard()
{
	LoggerAPI::LogDebug(StringUtil::Format("KernelMMR::Discard - Discarding changes since last flush."));
	const bool hashDiscard = m_pHashFile->Discard();
	const bool dataDiscard = m_pDataFile->Discard();

	return hashDiscard && dataDiscard;
}

bool KernelMMR::ApplyKernel(const TransactionKernel& kernel)
{
	// Add to data file
	Serializer serializer;
	kernel.Serialize(serializer);
	m_pDataFile->AddData(serializer.GetBytes());

	// Add hashes
	MMRHashUtil::AddHashes(*m_pHashFile, serializer.GetBytes(), nullptr);

	return true;
}