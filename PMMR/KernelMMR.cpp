#include "KernelMMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <StringUtil.h>
#include <Infrastructure/Logger.h>

KernelMMR::KernelMMR(const Config& config, HashFile* pHashFile, LeafSet&& leafSet, DataFile<KERNEL_SIZE>* pDataFile)
	: m_config(config),
	m_pHashFile(pHashFile),
	m_leafSet(std::move(leafSet)),
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

KernelMMR* KernelMMR::Load(const Config& config)
{
	HashFile* pHashFile = new HashFile(config.GetTxHashSetDirectory() + "kernel/pmmr_hash.bin");
	pHashFile->Load();

	LeafSet leafSet(config.GetTxHashSetDirectory() + "kernel/pmmr_leaf.bin");
	leafSet.Load();

	DataFile<KERNEL_SIZE>* pDataFile = new DataFile<KERNEL_SIZE>(config.GetTxHashSetDirectory() + "kernel/pmmr_data.bin");
	pDataFile->Load();

	return new KernelMMR(config, pHashFile, std::move(leafSet), pDataFile);
}

Hash KernelMMR::Root(const uint64_t mmrIndex) const
{
	return MMRHashUtil::Root(*m_pHashFile, mmrIndex, nullptr);
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

bool KernelMMR::Rewind(const uint64_t lastMMRIndex)
{
	const bool hashRewind = m_pHashFile->Rewind(lastMMRIndex);
	const bool dataRewind = m_pDataFile->Rewind(MMRUtil::GetNumLeaves(lastMMRIndex));

	m_leafSet.Rewind(MMRUtil::GetNumLeaves(lastMMRIndex), Roaring());

	return hashRewind && dataRewind;
}

bool KernelMMR::Flush()
{
	LoggerAPI::LogTrace(StringUtil::Format("KernelMMR::Flush - Flushing with size (%llu)", GetSize()));
	const bool hashFlush = m_pHashFile->Flush();
	const bool dataFlush = m_pDataFile->Flush();
	const bool leafSetFlush = m_leafSet.Flush();

	return hashFlush && dataFlush && leafSetFlush;
}

bool KernelMMR::Discard()
{
	LoggerAPI::LogDebug(StringUtil::Format("KernelMMR::Discard - Discarding changes since last flush."));
	const bool hashDiscard = m_pHashFile->Discard();
	const bool dataDiscard = m_pDataFile->Discard();
	m_leafSet.Discard();

	return hashDiscard && dataDiscard;
}

bool KernelMMR::ApplyKernel(const TransactionKernel& kernel)
{
	// Update leafset
	const uint64_t position = m_pHashFile->GetSize();
	m_leafSet.Add(position);

	// Add to data file
	Serializer serializer;
	kernel.Serialize(serializer);
	m_pDataFile->AddData(serializer.GetBytes());

	// Add hashes
	MMRHashUtil::AddHashes(*m_pHashFile, serializer.GetBytes(), nullptr);

	return true;
}