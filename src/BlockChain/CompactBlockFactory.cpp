#include "CompactBlockFactory.h"

#include <Common/Util/FunctionalUtil.h>
#include <Crypto/CSPRNG.h>

CompactBlock CompactBlockFactory::CreateCompactBlock(const FullBlock& block)
{
	const std::vector<TransactionOutput>& blockOutputs = block.GetOutputs();
	const std::vector<TransactionKernel>& blockKernels = block.GetKernels();

	// Get Coinbase Outputs
	std::vector<TransactionOutput> coinbaseOutputs;
	std::copy_if(
		blockOutputs.cbegin(),
		blockOutputs.cend(),
		std::back_inserter(coinbaseOutputs),
		[](const TransactionOutput& output) { return output.IsCoinbase(); }
	);

	// Get Coinbase Kernels
	std::vector<TransactionKernel> coinbaseKernels;
	std::copy_if(
		blockKernels.cbegin(),
		blockKernels.cend(),
		std::back_inserter(coinbaseKernels),
		[](const TransactionKernel& kernel) { return kernel.IsCoinbase(); }
	);

	// Get ShortIds
	const uint64_t nonce = CSPRNG::GenerateRandom(0, UINT64_MAX);
	std::vector<ShortId> kernelIds;
	FunctionalUtil::transform_if(
		blockKernels.cbegin(),
		blockKernels.cend(),
		std::back_inserter(kernelIds),
		[](const TransactionKernel& kernel) { return !kernel.IsCoinbase(); },
		[&block, nonce](const TransactionKernel& kernel) { return ShortId::Create(kernel.GetHash(), block.GetHash(), nonce); }
	);

	// Sort All
	std::sort(coinbaseOutputs.begin(), coinbaseOutputs.end(), SortOutputsByHash);
	std::sort(coinbaseKernels.begin(), coinbaseKernels.end(), SortKernelsByHash);
	std::sort(kernelIds.begin(), kernelIds.end(), SortShortIdsByHash);

	return CompactBlock(block.GetHeader(), nonce, std::move(coinbaseOutputs), std::move(coinbaseKernels), std::move(kernelIds));
}