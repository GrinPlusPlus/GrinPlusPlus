#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletTx.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <Wallet/Models/DTOs/WalletBalanceDTO.h>
#include <json/json.h>
#include <cstdint>
#include <vector>

class WalletSummaryDTO
{
public:
	WalletSummaryDTO(const uint64_t minimumConfirmations, const WalletBalanceDTO& balance, std::vector<WalletTx>&& transactions)
		: m_minimumConfirmations(minimumConfirmations), m_balance(balance), m_transactions(std::move(transactions)) { }

	uint64_t GetMinimumConfirmations() const { return m_minimumConfirmations; }
	const WalletBalanceDTO& GetBalance() const noexcept { return m_balance; }
	const std::vector<WalletTx>& GetTransactions() const { return m_transactions; }

	Json::Value ToJSON() const
	{
		Json::Value summaryJSON;
		summaryJSON["last_confirmed_height"] = m_balance.GetBlockHeight();
		summaryJSON["minimum_confirmations"] = m_minimumConfirmations;
		summaryJSON["total"] = m_balance.GetTotal();
		summaryJSON["amount_awaiting_confirmation"] = m_balance.GetUnconfirmed();
		summaryJSON["amount_immature"] = m_balance.GetImmature();
		summaryJSON["amount_locked"] = m_balance.GetLocked();
		summaryJSON["amount_currently_spendable"] = m_balance.GetSpendable();
	
		Json::Value transactionsJSON;
		for (const WalletTx& transaction : m_transactions)
		{
			transactionsJSON.append(WalletTxDTO(transaction, {}, {}, {}, "").ToJSON());
		}
		summaryJSON["transactions"] = transactionsJSON;

		return summaryJSON;
	}

private:
	uint64_t m_minimumConfirmations;
	WalletBalanceDTO m_balance;
	std::vector<WalletTx> m_transactions;
};