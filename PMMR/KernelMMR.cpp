#include "KernelMMR.h"
#include "Common/MMRUtil.h"

#include <StringUtil.h>
#include <Infrastructure/Logger.h>

KernelMMR::KernelMMR(const Config& config, HashFile&& hashFile, LeafSet&& leafSet, DataFile<KERNEL_SIZE>&& dataFile)
	: m_config(config),
	m_hashFile(std::move(hashFile)),
	m_leafSet(std::move(leafSet)),
	m_dataFile(std::move(dataFile))
{

}

KernelMMR* KernelMMR::Load(const Config& config)
{
	HashFile hashFile(config.GetTxHashSetDirectory() + "kernel/pmmr_hash.bin");
	hashFile.Load();

	LeafSet leafSet(config.GetTxHashSetDirectory() + "kernel/pmmr_leaf.bin");
	leafSet.Load();

	DataFile<KERNEL_SIZE> dataFile(config.GetTxHashSetDirectory() + "kernel/pmmr_data.bin");
	dataFile.Load();

	return new KernelMMR(config, std::move(hashFile), std::move(leafSet), std::move(dataFile));
}

Hash KernelMMR::Root(const uint64_t mmrIndex) const
{
	return m_hashFile.Root(mmrIndex);
}

std::unique_ptr<TransactionKernel> KernelMMR::GetKernelAt(const uint64_t mmrIndex) const
{
	if (MMRUtil::IsLeaf(mmrIndex))
	{
		const uint64_t numLeaves = MMRUtil::GetNumLeaves(mmrIndex);

		std::vector<unsigned char> data;
		m_dataFile.GetDataAt(numLeaves - 1, data);

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
	const bool hashRewind = m_hashFile.Rewind(lastMMRIndex);
	const bool dataRewind = m_dataFile.Rewind(MMRUtil::GetNumLeaves(lastMMRIndex));
	// TODO: Rewind leafset?

	return hashRewind && dataRewind;
}

bool KernelMMR::Flush()
{
	LoggerAPI::LogInfo(StringUtil::Format("KernelMMR::Flush - Flushing with size (%lld)", GetSize()));
	const bool hashFlush = m_hashFile.Flush();
	const bool dataFlush = m_dataFile.Flush();
	const bool leafSetFlush = m_leafSet.Flush();

	return hashFlush && dataFlush && leafSetFlush;
}