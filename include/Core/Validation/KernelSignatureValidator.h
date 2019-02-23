#pragma once

#include <Crypto/Crypto.h>
#include <Core/Models/TransactionKernel.h>
#include <Common/Util/HexUtil.h>
#include <Infrastructure/Logger.h>

class KernelSignatureValidator
{
public:
	// Verify the tx kernels.
	// No ability to batch verify these right now so just do them individually.
	static bool VerifyKernelSignatures(const std::vector<TransactionKernel>& kernels)
	{
		// Verify the transaction proof validity. Entails handling the commitment as a public key and checking the signature verifies with the fee as message.
		for (const TransactionKernel& kernel : kernels)
		{
			const Commitment& publicKey = kernel.GetExcessCommitment();
			const Signature& signature = kernel.GetExcessSignature();
			const Hash signatureMessage = kernel.GetSignatureMessage();

			if (!Crypto::VerifyKernelSignature(signature, publicKey, signatureMessage))
			{
				LoggerAPI::LogError("KernelSignatureValidator::VerifyKernelSignatures - Failed to verify kernel " + HexUtil::ConvertToHex(kernel.GetHash().GetData()));
				return false;
			}
		}

		return true;
	}
};