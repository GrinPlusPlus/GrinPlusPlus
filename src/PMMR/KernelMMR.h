#pragma once

#include "Common/MMR.h"
#include "Common/HashFile.h"

#include <Core/File/DataFile.h>
#include <Core/Models/TransactionKernel.h>
#include <Core/Traits/Lockable.h>
#include <Crypto/Hash.h>
#include <filesystem.h>
#include <stdint.h>

#define KERNEL_SIZE 114

class KernelMMR : public MMR
{
public:
	static std::shared_ptr<KernelMMR> Load(const fs::path& txHashSetPath);
	virtual ~KernelMMR() = default;

	std::unique_ptr<TransactionKernel> GetKernelAt(const uint64_t mmrIndex) const;
	bool Rewind(const uint64_t size);

	virtual Hash Root(const uint64_t size) const override final;
	virtual uint64_t GetSize() const override final { return m_pHashFile->GetSize(); }
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final { return std::make_unique<Hash>(m_pHashFile->GetDataAt(mmrIndex)); }
	virtual std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const override final;

	virtual void Commit() override final;
	virtual void Rollback() override final;

	void ApplyKernel(const TransactionKernel& kernel);

private:
	KernelMMR(std::shared_ptr<HashFile> pHashFile, std::shared_ptr<DataFile<KERNEL_SIZE>> pDataFile);

	mutable std::shared_ptr<HashFile> m_pHashFile;
	mutable std::shared_ptr<DataFile<KERNEL_SIZE>> m_pDataFile;
};