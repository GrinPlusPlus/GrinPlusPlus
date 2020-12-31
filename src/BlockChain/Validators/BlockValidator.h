#pragma once

#include <Core/Models/FullBlock.h>
#include <Core/Models/BlockSums.h>
#include <Database/BlockDb.h>
#include <Core/Traits/Lockable.h>
#include <Config/Config.h>
#include <PMMR/TxHashSet.h>
#include <memory>

class BlockValidator
{
public:
	static void VerifySelfConsistent(const FullBlock& block);

private:
	static void VerifyBody(const FullBlock& block);
	static void VerifyWeight(const FullBlock& block);
	static void VerifyKernelLockHeights(const FullBlock& block);
	static void VerifyCoinbase(const FullBlock& block);
};