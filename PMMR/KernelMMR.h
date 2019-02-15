#pragma once

#include "Common/MMR.h"
#include "Common/LeafSet.h"
#include "Common/HashFile.h"
#include "Common/DataFile.h"

#include <Core/TransactionKernel.h>
#include <Hash.h>
#include <Config/Config.h>
#include <stdint.h>

#define KERNEL_SIZE 114

class KernelMMR : public MMR
{
public:
	static KernelMMR* Load(const Config& config);
	~KernelMMR();

	std::unique_ptr<TransactionKernel> GetKernelAt(const uint64_t mmrIndex) const;
	bool Rewind(const uint64_t size);

	virtual Hash Root(const uint64_t lastMMRIndex) const override final;
	virtual uint64_t GetSize() const override final { return m_pHashFile->GetSize(); }
	virtual std::unique_ptr<Hash> GetHashAt(const uint64_t mmrIndex) const override final { return std::make_unique<Hash>(m_pHashFile->GetHashAt(mmrIndex)); }

	virtual bool Flush() override final;
	virtual bool Discard() override final;

	bool ApplyKernel(const TransactionKernel& kernel);

private:
	KernelMMR(const Config& config, HashFile* pHashFile, LeafSet&& leafSet, DataFile<KERNEL_SIZE>* pDataFile);

	const Config& m_config;
	HashFile* m_pHashFile;
	LeafSet m_leafSet;
	DataFile<KERNEL_SIZE>* m_pDataFile;
};