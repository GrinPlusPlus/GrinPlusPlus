#pragma once

#include "../WalletImpl.h"

#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Wallet/Exceptions/InsufficientFundsException.h>

class OutputBuilder
{
public:
	static std::vector<OutputDataEntity> CreateOutputs(
		std::shared_ptr<WalletImpl> pWallet,
		std::shared_ptr<IWalletDB> pBatch,
		const SecureVector& masterSeed, 
		const uint64_t totalAmount, 
		const uint32_t walletTxId, 
		const uint8_t numOutputs,
		const EBulletproofType& bulletproofType)
	{
		std::vector<OutputDataEntity> outputs;
		for (uint8_t i = 0; i < numOutputs; i++)
		{
			// If 3 outputs are requested for 11 nanogrins, the first output will contain 5, while the others contain 3.
			uint64_t coinAmount = (totalAmount / numOutputs);
			if (i == 0)
			{
				coinAmount += (totalAmount % numOutputs);
			}

			KeyChainPath keyChainPath = pBatch->GetNextChildPath(pWallet->GetUserPath());
			outputs.emplace_back(pWallet->CreateBlindedOutput(masterSeed, coinAmount, keyChainPath, walletTxId, bulletproofType));
		}

		return outputs;
	}
};