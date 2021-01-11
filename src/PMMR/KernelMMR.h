#pragma once

#include "Common/MMR.h"
#include "Common/HashFile.h"

#include <Core/File/DataFile.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Models/FullBlock.h>
#include <Core/Traits/Lockable.h>
#include <Crypto/Hash.h>
#include <filesystem.h>
#include <cstdint>

#define KERNEL_SIZE 114

class KernelMMR : public MMR
{
public:
	static std::shared_ptr<KernelMMR> Load(const fs::path& txHashSetPath);
	virtual ~KernelMMR() = default;

	std::unique_ptr<TransactionKernel> GetKernelAt(const uint64_t mmrIndex) const;
	bool Rewind(const uint64_t size);

	Hash Root(const uint64_t size) const final;
	uint64_t GetSize() const final { return m_pHashFile->GetSize(); }
	std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const final { return std::make_unique<Hash>(m_pHashFile->GetDataAt(mmrIndex)); }
	std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const final;

	void Commit() final;
	void Rollback() noexcept final;

	void ApplyKernel(const TransactionKernel& kernel);

private:
	KernelMMR(std::shared_ptr<HashFile> pHashFile, std::shared_ptr<DataFile<KERNEL_SIZE>> pDataFile);

	mutable std::shared_ptr<HashFile> m_pHashFile;
	mutable std::shared_ptr<DataFile<KERNEL_SIZE>> m_pDataFile;
};