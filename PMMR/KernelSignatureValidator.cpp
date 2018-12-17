#include "KernelSignatureValidator.h"
#include "Common/MMRUtil.h"

#include <Core/TransactionKernel.h>
#include <Serialization/Serializer.h>
#include <Crypto.h>

bool KernelSignatureValidator::ValidateKernelSignatures(const KernelMMR& kernelMMR) const
{
	uint64_t validatedKernels = 0;
	const uint64_t mmrSize = kernelMMR.GetSize();
	const uint64_t numKernels = MMRUtil::GetNumLeaves(mmrSize);
	for (uint64_t i = 0; i < mmrSize; i++)
	{
		std::unique_ptr<TransactionKernel> pKernel = kernelMMR.GetKernelAt(i);
		if (pKernel != nullptr)
		{
			if (!ValidateKernelSignature(*pKernel))
			{
				return false;
			}

			++validatedKernels;
		}
	}

	return true;
}

bool KernelSignatureValidator::ValidateKernelSignature(const TransactionKernel& kernel) const
{
	const Commitment& publicKey = kernel.GetExcessCommitment();
	const Signature& signature = kernel.GetExcessSignature();

	// Build message
	Serializer serializer(32);
	serializer.AppendBigInteger<16>(CBigInteger<16>::ValueOf(0));
	serializer.Append<uint64_t>(kernel.GetFee());
	serializer.Append<uint64_t>(kernel.GetLockHeight());
	const std::vector<unsigned char>& message = serializer.GetBytes();

	if (!Crypto::VerifyKernelSignature(signature, publicKey, message))
	{
		return false;
	}

	return true;
}