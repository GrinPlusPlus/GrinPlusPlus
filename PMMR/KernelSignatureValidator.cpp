#include "KernelSignatureValidator.h"
#include "Common/MMRUtil.h"

#include <Core/TransactionKernel.h>
#include <Serialization/Serializer.h>
#include <Infrastructure/Logger.h>
#include <HexUtil.h>
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

			// TODO: Verify the following:
			//let valid_features = match features{
			//KernelFeatures::COINBASE = > fee == 0 && lock_height == 0,
			//KernelFeatures::PLAIN = > lock_height == 0,
			//KernelFeatures::HEIGHT_LOCKED = > true,
			//_ = > false,
			//};
			//if !valid_features{
			//	return Err(Error::InvalidKernelFeatures);
			//}

			const Commitment& publicKey = pKernel->GetExcessCommitment();
			const Signature& signature = pKernel->GetExcessSignature();
			const Hash signatureMessage = pKernel->GetSignatureMessage();

			if (!Crypto::VerifyKernelSignature(signature, publicKey, signatureMessage))
			{
				LoggerAPI::LogError("KernelSignatureValidator::ValidateKernelSignatures - Failed to verify kernel " + HexUtil::ConvertHash(pKernel->GetHash()));
				return false;
			}

			++validatedKernels;
		}
	}

	return true;
}