#pragma once

#include <Core/Util/JsonUtil.h>
#include <Wallet/WalletTx.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <json/json.h>
#include <stdint.h>
#include <vector>

class WalletSummary
{
public:
	WalletSummary(const uint64_t lastConfirmedHeight, const uint64_t minimumConfirmations, const uint64_t total, const uint64_t unconfirmed, 
		const uint64_t immature, const uint64_t locked, const uint64_t spendable, std::vector<WalletTx>&& transactions)
		: m_lastConfirmedHeight(lastConfirmedHeight),
		m_minimumConfirmations(minimumConfirmations),
		m_total(total),
		m_amountAwaitingConfirmation(unconfirmed),
		m_amountImmature(immature),
		m_amountLocked(locked),
		m_amountCurrentlySpendable(spendable),
		m_transactions(std::move(transactions))
	{

	}

	inline uint64_t GetLastConfirmedHeight() const { return m_lastConfirmedHeight; }
	inline uint64_t GetMinimumConfirmations() const { return m_minimumConfirmations; }
	inline uint64_t GetTotal() const { return m_total; }
	inline uint64_t GetAmountAwaitingConfirmation() const { return m_amountAwaitingConfirmation; }
	inline uint64_t GetAmountImmature() const { return m_amountImmature; }
	inline uint64_t GetAmountLocked() const { return m_amountLocked; }
	inline uint64_t GetAmountCurrentlySpendable() const { return m_amountCurrentlySpendable; }
	inline const std::vector<WalletTx>& GetTransactions() const { return m_transactions; }

	Json::Value ToJSON() const
	{
		Json::Value summaryJSON;
		summaryJSON["last_confirmed_height"] = m_lastConfirmedHeight;
		summaryJSON["minimum_confirmations"] = m_minimumConfirmations;
		summaryJSON["total"] = m_amountCurrentlySpendable + m_amountAwaitingConfirmation + m_amountImmature;
		summaryJSON["amount_awaiting_confirmation"] = m_amountAwaitingConfirmation;
		summaryJSON["amount_immature"] = m_amountImmature;
		summaryJSON["amount_locked"] = m_amountLocked;
		summaryJSON["amount_currently_spendable"] = m_amountCurrentlySpendable;
	
		Json::Value transactionsJSON;
		for (const WalletTx& transaction : m_transactions)
		{
			transactionsJSON.append(WalletTxDTO(transaction, std::vector<WalletOutputDTO>()).ToJSON());
		}
		summaryJSON["transactions"] = transactionsJSON;

		return summaryJSON;
	}

private:
	uint64_t m_lastConfirmedHeight;
	uint64_t m_minimumConfirmations;
	uint64_t m_total;
	uint64_t m_amountAwaitingConfirmation;
	uint64_t m_amountImmature;
	uint64_t m_amountLocked;
	uint64_t m_amountCurrentlySpendable;
	std::vector<WalletTx> m_transactions;
};