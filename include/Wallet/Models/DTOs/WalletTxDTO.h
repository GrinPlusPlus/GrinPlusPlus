#pragma once

#include <Common/Util/TimeUtil.h>
#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletTx.h>
#include <Wallet/Models/DTOs/WalletOutputDTO.h>
#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/Models/Slatepack/SlatepackMessage.h>

class WalletTxDTO
{
public:
	WalletTxDTO(
		const WalletTx& walletTx,
		const std::vector<Commitment>& kernels,
		const std::vector<WalletOutputDTO>& outputs,
		const std::optional<Slate>& slate,
		const std::string& armored_slatepack
	) : m_walletTx(walletTx),
		m_kernels(kernels),
		m_outputs(outputs),
		m_slate(slate),
		m_armored(armored_slatepack) { }

	uint32_t GetId() const noexcept { return m_walletTx.GetId(); }
	const WalletTx& GetTx() const noexcept { return m_walletTx; }
	const std::vector<Commitment>& GetKernels() const noexcept { return m_kernels; }
	const std::vector<WalletOutputDTO>& GetOutputs() const noexcept { return m_outputs; }
	const std::optional<Slate>& GetSlate() const noexcept { return m_slate; }
	const std::string& GetArmoredSlatepack() const noexcept { return m_armored; }

	Json::Value ToJSON() const
	{
		Json::Value transactionJSON;
		transactionJSON["id"] = m_walletTx.GetId();
		transactionJSON["type"] = WalletTxType::ToString(m_walletTx.GetType());
		transactionJSON["amount_credited"] = m_walletTx.GetAmountCredited();
		transactionJSON["amount_debited"] = m_walletTx.GetAmountDebited();
		transactionJSON["creation_date_time"] = TimeUtil::ToSeconds(m_walletTx.GetCreationTime());

		JsonUtil::AddOptionalField(transactionJSON, "address", m_walletTx.GetAddress());
		JsonUtil::AddOptionalField(transactionJSON, "slate_message", m_walletTx.GetSlateMessage());
		JsonUtil::AddOptionalField(transactionJSON, "confirmed_height", m_walletTx.GetConfirmationHeight());

		if (m_walletTx.GetFee().has_value()) {
			transactionJSON["fee"] = m_walletTx.GetFee().value().ToJSON();
		}

		if (m_walletTx.GetSlateId().has_value()) {
			transactionJSON["slate_id"] = uuids::to_string(m_walletTx.GetSlateId().value());
		}

		if (m_walletTx.GetConfirmationTime().has_value()) {
			transactionJSON["confirmation_date_time"] = TimeUtil::ToSeconds(m_walletTx.GetConfirmationTime().value());
		}

		Json::Value kernelsJSON;
		for (const auto& kernel : m_kernels)
		{
			Json::Value kernelJSON;
			kernelJSON["commitment"] = kernel.ToHex();
			kernelsJSON.append(kernelJSON);
		}

		transactionJSON["kernels"] = kernelsJSON;

		Json::Value outputsJSON;
		for (const WalletOutputDTO& output : m_outputs)
		{
			outputsJSON.append(output.ToJSON());
		}

		transactionJSON["outputs"] = outputsJSON;

		if (m_slate.has_value()) {
			transactionJSON["slate"] = m_slate.value().ToJSON();
		}

		transactionJSON["armored_slatepack"] = m_armored;

		return transactionJSON;
	}

private:
	WalletTx m_walletTx;
	std::vector<Commitment> m_kernels;
	std::vector<WalletOutputDTO> m_outputs;
	std::optional<Slate> m_slate;
	std::string m_armored;
};