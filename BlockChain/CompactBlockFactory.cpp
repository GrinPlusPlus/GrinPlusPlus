#include "CompactBlockFactory.h"

#include <Crypto/RandomNumberGenerator.h>

CompactBlock CompactBlockFactory::CreateCompactBlock(const FullBlock& block)
{
	const uint64_t nonce = RandomNumberGenerator::GenerateRandom(0, UINT64_MAX);

	std::vector<TransactionOutput> coinbaseOutputs;
	for (const TransactionOutput& output : block.GetTransactionBody().GetOutputs())
	{
		if (output.GetFeatures() == EOutputFeatures::COINBASE_OUTPUT)
		{
			coinbaseOutputs.push_back(output);
		}
	}

	std::vector<TransactionKernel> coinbaseKernels;
	std::vector<ShortId> kernelIds;
	for (const TransactionKernel& kernel : block.GetTransactionBody().GetKernels())
	{
		if (kernel.GetFeatures() == EKernelFeatures::COINBASE_KERNEL)
		{
			coinbaseKernels.push_back(kernel);
		}
		else
		{
			kernelIds.push_back(ShortId::Create(kernel.GetHash(), block.GetHash(), nonce));
		}
	}

	std::sort(coinbaseOutputs.begin(), coinbaseOutputs.end(), SortOutputsByHash);
	std::sort(coinbaseKernels.begin(), coinbaseKernels.end(), SortKernelsByHash);
	std::sort(kernelIds.begin(), kernelIds.end(), SortShortIdsByHash);

	return CompactBlock(BlockHeader(block.GetBlockHeader()), nonce, std::move(coinbaseOutputs), std::move(coinbaseKernels), std::move(kernelIds));
}