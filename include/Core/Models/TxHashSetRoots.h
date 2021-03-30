#pragma once

#include <Crypto/Models/Hash.h>

struct MMRInfo
{
    Hash root;
    uint64_t size;
};

class TxHashSetRoots
{
public:
    TxHashSetRoots(const MMRInfo& kernelInfo, const MMRInfo& outputInfo, const MMRInfo& rangeProofInfo)
        : m_kernelInfo(kernelInfo), m_outputInfo(outputInfo), m_rangeProofInfo(rangeProofInfo) { }

    const MMRInfo& GetKernelInfo() const noexcept { return m_kernelInfo; }
    const MMRInfo& GetOutputInfo() const noexcept { return m_outputInfo; }
    const MMRInfo& GetRangeProofInfo() const noexcept { return m_rangeProofInfo; }

private:
    MMRInfo m_kernelInfo;
    MMRInfo m_outputInfo;
    MMRInfo m_rangeProofInfo;
};