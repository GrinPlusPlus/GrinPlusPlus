#include "KernelMMR.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Core/Global.h>
#include <Core/Traits/Lockable.h>
#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>

KernelMMR::KernelMMR(std::shared_ptr<HashFile> pHashFile, std::shared_ptr<DataFile<KERNEL_SIZE>> pDataFile)
	: m_pHashFile(pHashFile),
	m_pDataFile(pDataFile)
{

}

std::shared_ptr<KernelMMR> KernelMMR::Load(const fs::path& txHashSetPath)
{
	auto pHashFile = HashFile::Load(txHashSetPath / "kernel" / "pmmr_hash.bin");
	auto pDataFile = DataFile<KERNEL_SIZE>::Load(txHashSetPath / "kernel" / "pmmr_data.bin");

	auto pKernelMMR =  std::make_shared<KernelMMR>(pHashFile, pDataFile);

	if (pHashFile->GetSize() == 0) {
		pKernelMMR->ApplyKernel(Global::GetGenesisBlock().GetKernels().front());
	}

	return pKernelMMR;
}

Hash KernelMMR::Root(const uint64_t size) const
{
	return MMRHashUtil::Root(m_pHashFile, size, nullptr);
}

std::unique_ptr<TransactionKernel> KernelMMR::GetKernelAt(const LeafIndex& leaf_idx) const
{
	std::vector<uint8_t> data = m_pDataFile->GetDataAt(leaf_idx.Get());
	if (data.size() == KERNEL_SIZE) {
		ByteBuffer byteBuffer(std::move(data));
		return std::make_unique<TransactionKernel>(TransactionKernel::Deserialize(byteBuffer));
	}

	return std::unique_ptr<TransactionKernel>(nullptr);
}

std::vector<Hash> KernelMMR::GetLastLeafHashes(const uint64_t numHashes) const
{
	return MMRHashUtil::GetLastLeafHashes(m_pHashFile, nullptr, nullptr, numHashes);
}

bool KernelMMR::Rewind(const uint64_t num_kernels)
{
	m_pHashFile->Rewind(LeafIndex::At(num_kernels).GetPosition());
	m_pDataFile->Rewind(num_kernels);
	return true;
}

void KernelMMR::Commit()
{
	m_pHashFile->Commit();
	m_pDataFile->Commit();
}

void KernelMMR::Rollback() noexcept
{
	m_pHashFile->Rollback();
	m_pDataFile->Rollback();
}

void KernelMMR::ApplyKernel(const TransactionKernel& kernel)
{
	std::vector<uint8_t> serialized = kernel.Serialized();

	m_pDataFile->AddData(serialized);
	MMRHashUtil::AddHashes(m_pHashFile, serialized, nullptr);
}