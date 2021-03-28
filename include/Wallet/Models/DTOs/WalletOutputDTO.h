#pragma once

#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Core/Util/JsonUtil.h>

class WalletOutputDTO
{
public:
	WalletOutputDTO(
		KeyChainPath&& keyChainPath,
		Commitment&& commitment,
		const uint64_t amount,
		const EOutputStatus status,
		const std::optional<uint64_t>& mmrIndexOpt,
		const std::optional<uint64_t>& blockHeightOpt,
		const std::optional<uint32_t>& walletTxIdOpt,
		const std::optional<std::string>& labelOpt
	)
		: m_path(std::move(keyChainPath)),
		m_commitment(std::move(commitment)),
		m_amount(amount),
		m_status(status),
		m_mmrIndexOpt(mmrIndexOpt),
		m_heightOpt(blockHeightOpt),
		m_walletIdOpt(walletTxIdOpt),
		m_labelOpt(labelOpt) { }

	const KeyChainPath& GetPath() const noexcept { return m_path; }
	const Commitment& GetCommitment() const noexcept { return m_commitment; }
	uint64_t GetAmount() const noexcept { return m_amount; }
	EOutputStatus GetStatus() const noexcept { return m_status; }
	const std::optional<uint64_t>& GetMMRIndex() const noexcept { return m_mmrIndexOpt; }
	const std::optional<uint64_t>& GetBlockHeight() const noexcept { return m_heightOpt; }
	const std::optional<uint32_t>& GetWalletTxId() const noexcept { return m_walletIdOpt; }
	const std::optional<std::string>& GetLabel() const noexcept { return m_labelOpt; }

	static WalletOutputDTO FromOutputData(const OutputDataEntity& entity)
	{
		auto path = entity.GetKeyChainPath();
		auto commitment = entity.GetOutput().GetCommitment();
		auto amount = entity.GetAmount();
		auto status = entity.GetStatus();

		auto mmrIndexOpt = entity.GetMMRIndex();
		auto blockHeightOpt = entity.GetBlockHeight();
		auto walletIdOpt = entity.GetWalletTxId();
		auto labels = entity.GetLabels();

		return WalletOutputDTO(
			std::move(path),
			std::move(commitment),
			amount,
			status,
			mmrIndexOpt,
			blockHeightOpt,
			walletIdOpt,
			labels.empty() ? std::nullopt : std::make_optional(labels.front())
		);
	}

	static WalletOutputDTO FromJSON(const Json::Value& json)
	{
		auto path = KeyChainPath::FromString(JsonUtil::GetRequiredString(json, "keychain_path"));
		auto commitment = JsonUtil::GetCommitment(json, "commitment");
		auto amount = JsonUtil::GetRequiredUInt64(json, "amount");
		auto status = OutputStatus::FromString(JsonUtil::GetRequiredString(json, "status"));

		auto mmrIndexOpt = JsonUtil::GetUInt64Opt(json, "mmr_index");
		auto blockHeightOpt = JsonUtil::GetUInt64Opt(json, "block_height");
		auto walletIdOpt = JsonUtil::GetUInt32Opt(json, "transaction_id");
		auto labelOpt = JsonUtil::GetStringOpt(json, "label");

		return WalletOutputDTO(
			std::move(path),
			std::move(commitment),
			amount,
			status,
			mmrIndexOpt,
			blockHeightOpt,
			walletIdOpt,
			labelOpt
		);
	}

	Json::Value ToJSON() const
	{
		Json::Value outputJSON;

		outputJSON["keychain_path"] = m_path.Format();
		outputJSON["commitment"] = m_commitment.ToHex();
		outputJSON["amount"] = m_amount;
		outputJSON["status"] = OutputStatus::ToString(m_status);

		JsonUtil::AddOptionalField(outputJSON, "mmr_index", m_mmrIndexOpt);
		JsonUtil::AddOptionalField(outputJSON, "block_height", m_heightOpt);
		JsonUtil::AddOptionalField(outputJSON, "transaction_id", m_walletIdOpt);
		JsonUtil::AddOptionalField(outputJSON, "label", m_labelOpt);

		return outputJSON;
	}

private:
	KeyChainPath m_path;
	Commitment m_commitment;
	uint64_t m_amount;
	EOutputStatus m_status;

	std::optional<uint64_t> m_mmrIndexOpt;
	std::optional<uint64_t> m_heightOpt;
	std::optional<uint32_t> m_walletIdOpt;
	std::optional<std::string> m_labelOpt;
};