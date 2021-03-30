#pragma once

#include <Common/Secure.h>
#include <Crypto/Models/Commitment.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Wallet/WalletDB/WalletDB.h>
#include <Wallet/WalletTx.h>
#include <unordered_set>
#include <vector>

class CancelTx
{
public:
	static void CancelWalletTx(const SecureVector& masterSeed, const std::shared_ptr<IWalletDB>& pWalletDB, WalletTx& walletTx);

private:
	static std::vector<OutputDataEntity> GetOutputsToUpdate(
		std::vector<OutputDataEntity>& outputs,
		const std::unordered_set<Commitment>& inputCommitments,
		const WalletTx& walletTx
	);
};