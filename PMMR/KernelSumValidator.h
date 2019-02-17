#pragma once

#include "TxHashSetImpl.h"

#include <Core/Models/BlockHeader.h>
#include <Crypto/Commitment.h>
#include <Crypto/BlindingFactor.h>

#include <memory>

class KernelSumValidator
{
public:
	bool ValidateKernelSums(TxHashSet& txHashSet, const BlockHeader& blockHeader, Commitment& outputSumOut, Commitment& kernelSumOut) const;

private:
	std::unique_ptr<Commitment> AddCommitments(TxHashSet& txHashSet, const uint64_t overage, const uint64_t outputMMRSize) const;
	std::unique_ptr<Commitment> AddKernelExcesses(TxHashSet& txHashSet, const uint64_t kernelMMRSize) const;
	std::unique_ptr<Commitment> AddKernelOffset(const Commitment& kernelSum, const BlindingFactor& totalKernelOffset) const;
};