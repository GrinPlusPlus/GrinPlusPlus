#include "WalletTxLoader.h"

#include <Wallet/WalletDB/WalletDB.h>
#include <Common/Util/FunctionalUtil.h>

std::vector<WalletTxDTO> WalletTxLoader::LoadTransactions(
    const std::shared_ptr<const IWalletDB>& pWalletDB,
	const SecureVector& masterSeed,
    const ListTxsCriteria& criteria) const
{
	std::vector<OutputDataEntity> outputs = pWalletDB->GetOutputs(masterSeed);
	std::vector<WalletTx> walletTransactions = LoadWalletTxs(pWalletDB, masterSeed, criteria);

	std::vector<WalletTxDTO> walletTxDTOs;
	std::transform(
		walletTransactions.cbegin(), walletTransactions.cend(),
		std::back_inserter(walletTxDTOs),
		[this, &pWalletDB, &masterSeed, &outputs](const WalletTx& walletTx) {
			return ToDTO(pWalletDB, masterSeed, outputs, walletTx);
		}
	);

	return walletTxDTOs;
}

WalletTxDTO WalletTxLoader::ToDTO(
	const std::shared_ptr<const IWalletDB>& pWalletDB,
	const SecureVector& masterSeed,
	const std::vector<OutputDataEntity>& outputs,
	const WalletTx& walletTx) const
{
	std::vector<Commitment> kernels;
	auto txOpt = walletTx.GetTransaction();
	if (txOpt.has_value()) {
		FunctionalUtil::transform_if(
			txOpt.value().GetKernels().cbegin(), txOpt.value().GetKernels().cend(),
			std::back_inserter(kernels),
			[](const TransactionKernel& kernel) {
				return kernel.GetExcessCommitment() != CBigInteger<33>::ValueOf(0);
			},
			[](const TransactionKernel& kernel) { return kernel.GetExcessCommitment(); }
		);
	}

	std::vector<WalletOutputDTO> outputDTOs;
	FunctionalUtil::transform_if(
		outputs.cbegin(), outputs.cend(),
		std::back_inserter(outputDTOs),
		[&walletTx](const OutputDataEntity& output) {
			return output.GetWalletTxId().has_value() && output.GetWalletTxId().value() == walletTx.GetId();
		},
		[](const OutputDataEntity& output) { return WalletOutputDTO::FromOutputData(output); }
	);

	std::optional<Slate> slate = std::nullopt;

	std::string armored_slatepack;
	if (walletTx.GetSlateId().has_value()) {
		auto latest_slate = pWalletDB->LoadLatestSlate(masterSeed, walletTx.GetSlateId().value());
		if (latest_slate.first != nullptr) {
			slate = std::make_optional<Slate>(*latest_slate.first);
		}

		armored_slatepack = latest_slate.second;
	}

	return WalletTxDTO{ walletTx, kernels, outputDTOs, slate, armored_slatepack };
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
	if (walletTx.GetConfirmationTime().has_value() && walletTx.GetConfirmationTime().value() < txDateTime) {
		txDateTime = walletTx.GetConfirmationTime().value();
	}

	if (criteria.GetStartRange().has_value() && criteria.GetStartRange().value() > txDateTime) {
		return false;
	}

	if (criteria.GetEndRange().has_value() && criteria.GetEndRange().value() < txDateTime) {
		return false;
	}

	if (!criteria.GetTxIds().empty() && criteria.GetTxIds().count(walletTx.GetId()) == 0) {
		return false;
	}

	return criteria.GetStatuses().empty()
		|| criteria.GetStatuses().count(walletTx.GetType()) > 0;
}