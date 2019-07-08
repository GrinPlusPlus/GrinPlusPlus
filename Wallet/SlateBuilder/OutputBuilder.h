#pragma once

#include "../Wallet.h"

#include <Wallet/OutputData.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>

class OutputBuilder
{
public:
	static std::vector<OutputData> CreateOutputs(
		Wallet& wallet, 
		const SecureVector& masterSeed, 
		const uint64_t totalAmount, 
		const uint32_t walletTxId, 
		const uint8_t numOutputs,
		const EBulletproofType& bulletproofType)
	{
		if (totalAmount < numOutputs)
		{
			throw InsufficientFundsException();
		}

		std::vector<OutputData> outputs;
		for (uint8_t i = 0; i < numOutputs; i++)
		{
			// If 3 outputs are requested for 11 nanogrins, the first output will contain 5, while the others contain 3.
			uint64_t coinAmount = (totalAmount / numOutputs);
			if (i == 0)
			{
				coinAmount += (totalAmount % numOutputs);
			}

			outputs.emplace_back(wallet.CreateBlindedOutput(masterSeed, coinAmount, walletTxId, bulletproofType));
		}

		return outputs;
	}
};