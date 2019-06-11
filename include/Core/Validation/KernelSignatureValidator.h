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
		std::vector<const Commitment*> commitments;
		commitments.reserve(kernels.size());
		std::vector<const Signature*> signatures;
		signatures.reserve(kernels.size());
		std::vector<Hash> msgs;
		msgs.reserve(kernels.size());
		std::vector<const Hash*> messages;
		messages.reserve(kernels.size());

		// Verify the transaction proof validity. Entails handling the commitment as a public key and checking the signature verifies with the fee as message.
		for (size_t i = 0; i < kernels.size(); i++)
		{
			const TransactionKernel& kernel = kernels[i];
			commitments.push_back(&kernel.GetExcessCommitment());
			signatures.push_back(&kernel.GetExcessSignature());
			msgs.emplace_back(kernel.GetSignatureMessage());
			messages.push_back(&msgs[i]);
		}

		LoggerAPI::LogTrace("KernelSignatureValidator::VerifyKernelSignatures - Start verify");
		if (!Crypto::VerifyKernelSignatures(signatures, commitments, messages))
		{
			LoggerAPI::LogError("KernelSignatureValidator::VerifyKernelSignatures - Failed to verify kernels.");
			return false;
		}

		LoggerAPI::LogTrace("KernelSignatureValidator::VerifyKernelSignatures - Verify success");
		return true;
	}
};