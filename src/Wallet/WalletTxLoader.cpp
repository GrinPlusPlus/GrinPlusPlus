#include "WalletTxLoader.h"

#include <Common/Util/FunctionalUtil.h>

std::vector<WalletTxDTO> WalletTxLoader::LoadTransactions(
    const std::shared_ptr<const IWalletDB>& pWalletDB,
	const SecureVector& masterSeed,
    const ListTxsCriteria& criteria) const
{
	std::vector<OutputDataEntity> outputs = pWalletDB->GetOutputs(masterSeed);
	std::vector<WalletTx> walletTransactions = LoadWalletTxs(pWalletDB, masterSeed, criteria);

	std::vector<WalletTxDTO> walletTxDTOs;
	for (const WalletTx& walletTx : walletTransactions)
	{
		std::vector<Commitment> kernels;
		auto txOpt = walletTx.GetTransaction();
		if (txOpt.has_value())
		{
			FunctionalUtil::transform_if(
				txOpt.value().GetKernels().begin(), txOpt.value().GetKernels().end(),
				std::back_inserter(kernels),
				[](const TransactionKernel& kernel) {
					return kernel.GetExcessCommitment() != CBigInteger<33>::ValueOf(0);
				},
				[](const TransactionKernel& kernel) { return kernel.GetExcessCommitment(); }
			);
		}

		std::vector<WalletOutputDTO> outputDTOs;
		for (const OutputDataEntity& output : outputs)
		{
			if (output.GetWalletTxId().has_value() && output.GetWalletTxId().value() == walletTx.GetId())
			{
				outputDTOs.emplace_back(WalletOutputDTO(output));
			}
		}

		walletTxDTOs.emplace_back(WalletTxDTO(walletTx, kernels, outputDTOs));
	}

	return walletTxDTOs;
}

std::vector<WalletTx> WalletTxLoader::LoadWalletTxs(
	const std::shared_ptr<const IWalletDB>& pWalletDB,
	const SecureVector& masterSeed,
	const ListTxsCriteria& criteria) const
{
	std::vector<WalletTx> walletTxs = pWalletDB->GetTransactions(masterSeed);

	std::vector<WalletTx> filteredTxs;
	std::copy_if(
		walletTxs.cbegin(), walletTxs.cend(),
		std::back_inserter(filteredTxs),
		[this, &criteria](const WalletTx& walletTx) { return this->MeetsCriteria(walletTx, criteria); }
	);

	return filteredTxs;
}

bool WalletTxLoader::MeetsCriteria(const WalletTx& walletTx, const ListTxsCriteria& criteria) const
{
	std::chrono::system_clock::time_point txDateTime = walletTx.GetCreationTime();
	if (walletTx.GetConfirmationTime().has_value() && walletTx.GetConfirmationTime().value() < txDateTime)
	{
		txDateTime = walletTx.GetConfirmationTime().value();
	}

	if (criteria.GetStartRange().has_value() && criteria.GetStartRange().value() > txDateTime)
	{
		return false;
	}

	if (criteria.GetEndRange().has_value() && criteria.GetEndRange().value() < txDateTime)
	{
		return false;
	}

	return criteria.GetStatuses().empty()
		|| criteria.GetStatuses().count(walletTx.GetType()) > 0;
}