#pragma once

#include "Common/MMR.h"
#include "Common/HashFile.h"

#include <Core/DataFile.h>
#include <Core/Models/TransactionKernel.h>
#include <Crypto/Hash.h>
#include <stdint.h>

#define KERNEL_SIZE 114

class KernelMMR : public MMR
{
public:
	static KernelMMR* Load(const std::string& txHashSetDirectory);
	~KernelMMR();

	std::unique_ptr<TransactionKernel> GetKernelAt(const uint64_t mmrIndex) const;
	bool Rewind(const uint64_t size);

	virtual Hash Root(const uint64_t size) const override final;
	virtual uint64_t GetSize() const override final { return m_pHashFile->GetSize(); }
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final { return std::make_unique<Hash>(m_pHashFile->GetHashAt(mmrIndex)); }
	virtual std::vector<Hash> GetLastLeafHashes(const uint64_t numHashes) const override final;

	virtual bool Flush() override final;
	virtual bool Discard() override final;

	bool ApplyKernel(const TransactionKernel& kernel);

private:
	KernelMMR(HashFile* pHashFile, DataFile<KERNEL_SIZE>* pDataFile);

	HashFile* m_pHashFile;
	DataFile<KERNEL_SIZE>* m_pDataFile;
};