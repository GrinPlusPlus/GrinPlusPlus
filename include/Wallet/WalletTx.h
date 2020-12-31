#pragma once

#include <Wallet/WalletTxType.h>
#include <Wallet/Models/Slate/SlatePaymentProof.h>
#include <Core/Models/Transaction.h>
#include <Common/Util/TimeUtil.h>
#include <json/json.h>
#include <uuid.h>
#include <chrono>
#include <optional>

static const uint8_t WALLET_TX_DATA_FORMAT = 3;

class WalletTx
{
public:
	WalletTx(
		const uint32_t walletTxId,
		const EWalletTxType type,
		const std::optional<uuids::uuid>& slateIdOpt,
		const std::optional<std::string>& addressOpt,
		const std::optional<std::string>& slateMessageOpt,
		const std::chrono::system_clock::time_point& creationTime,
		const std::optional<std::chrono::system_clock::time_point>& confirmationTimeOpt,
		const std::optional<uint64_t>& confirmationHeightOpt,
		const uint64_t amountCredited,
		const uint64_t amountDebited,
		const std::optional<Fee>& feeOpt,
		const std::optional<Transaction>& transactionOpt,
		const std::optional<SlatePaymentProof>& paymentProofOpt
	)	
		: m_walletTxId(walletTxId),
		m_type(type),
		m_slateIdOpt(slateIdOpt),
		m_addressOpt(addressOpt),
		m_slateMessageOpt(slateMessageOpt),
		m_creationTime(creationTime),
		m_confirmationTimeOpt(confirmationTimeOpt),
		m_confirmedHeightOpt(confirmationHeightOpt),
		m_amountCredited(amountCredited),
		m_amountDebited(amountDebited),
		m_feeOpt(feeOpt),
		m_transactionOpt(transactionOpt),
		m_paymentProofOpt(paymentProofOpt)
	{

	}

	uint32_t GetId() const { return m_walletTxId; }
	EWalletTxType GetType() const { return m_type; }
	const std::optional<uuids::uuid>& GetSlateId() const { return m_slateIdOpt; }
	const std::optional<std::string>& GetAddress() const { return m_addressOpt; }
	const std::optional<std::string>& GetSlateMessage() const { return m_slateMessageOpt; }
	const std::chrono::system_clock::time_point& GetCreationTime() const { return m_creationTime; }
	const std::optional<std::chrono::system_clock::time_point>& GetConfirmationTime() const { return m_confirmationTimeOpt; }
	const std::optional<uint64_t>& GetConfirmationHeight() const { return m_confirmedHeightOpt; }
	uint64_t GetAmountCredited() const { return m_amountCredited; }
	uint64_t GetAmountDebited() const { return m_amountDebited; }
	std::optional<Fee> GetFee() const { return m_feeOpt; }
	const std::optional<Transaction>& GetTransaction() const { return m_transactionOpt; }
	const std::optional<SlatePaymentProof>& GetPaymentProof() const { return m_paymentProofOpt; }

	void SetType(const EWalletTxType type) { m_type = type; }
	void SetConfirmedHeight(const uint64_t blockHeight) { m_confirmedHeightOpt = std::make_optional(blockHeight); }
	void SetTransaction(const Transaction& transaction) { m_transactionOpt = std::make_optional(transaction); }

	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint8_t>(WALLET_TX_DATA_FORMAT);
		serializer.Append<uint32_t>(m_walletTxId);
		serializer.Append<uint8_t>((uint8_t)m_type);
		serializer.AppendVarStr(m_slateIdOpt.has_value() ? uuids::to_string(m_slateIdOpt.value()) : "");
		serializer.AppendVarStr(m_addressOpt.has_value() ? m_addressOpt.value() : "");
		serializer.AppendVarStr(m_slateMessageOpt.has_value() ? m_slateMessageOpt.value() : "");
		serializer.Append<int64_t>(TimeUtil::ToInt64(m_creationTime));
		serializer.Append<int64_t>((int64_t)(m_confirmationTimeOpt.has_value() ? TimeUtil::ToInt64(m_confirmationTimeOpt.value()) : 0));
		serializer.Append<uint64_t>(m_confirmedHeightOpt.value_or(0));
		serializer.Append<uint64_t>(m_amountCredited);
		serializer.Append<uint64_t>(m_amountDebited);

		if (m_feeOpt.has_value())
		{
			serializer.Append<uint8_t>(1);
			m_feeOpt.value().Serialize(serializer);
		}
		else
		{
			serializer.Append<uint8_t>(0);
		}

		// Payment proof addresses
		if (m_paymentProofOpt.has_value())
		{
			serializer.Append<uint8_t>(1);
			m_paymentProofOpt.value().Serialize(serializer);
		}
		else
		{
			serializer.Append<uint8_t>(0);
		}

		if (m_transactionOpt.has_value())
		{
			m_transactionOpt.value().Serialize(serializer);
		}
	}

	static WalletTx Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t walletTxFormat = byteBuffer.ReadU8();
		if (walletTxFormat > WALLET_TX_DATA_FORMAT)
		{
			throw DESERIALIZATION_EXCEPTION_F(
				"Expected format <= {}, but was {}",
				WALLET_TX_DATA_FORMAT,
				walletTxFormat
			);
		}

		const uint32_t walletTxId = byteBuffer.ReadU32();
		const EWalletTxType type = (EWalletTxType)byteBuffer.ReadU8();

		std::optional<uuids::uuid> slateIdOpt = std::nullopt;
		std::string slateIdStr = byteBuffer.ReadVarStr();
		if (!slateIdStr.empty())
		{
			slateIdOpt = uuids::uuid::from_string(slateIdStr);
		}

		std::optional<std::string> addressOpt = std::nullopt;
		if (walletTxFormat >= 2)
		{
			std::string address = byteBuffer.ReadVarStr();
			if (!address.empty())
			{
				addressOpt = std::make_optional(std::move(address));
			}
		}

		std::optional<std::string> slateMessageOpt = std::nullopt;
		if (walletTxFormat >= 1)
		{
			std::string slateMessage = byteBuffer.ReadVarStr();
			if (!slateMessage.empty())
			{
				slateMessageOpt = std::make_optional(std::move(slateMessage));
			}
		}

		std::chrono::system_clock::time_point creationTime = TimeUtil::ToTimePoint(byteBuffer.Read64());

		std::optional<std::chrono::system_clock::time_point> confirmationTimeOpt = std::nullopt;
		const int64_t confirmationTimeMillis = byteBuffer.Read64();
		if (confirmationTimeMillis != 0)
		{
			confirmationTimeOpt = std::make_optional(TimeUtil::ToTimePoint(confirmationTimeMillis));
		}

		std::optional<uint64_t> confirmedHeightOpt = std::nullopt;
		const uint64_t blockHeight = byteBuffer.ReadU64();
		if (blockHeight != 0)
		{
			confirmedHeightOpt = std::make_optional(blockHeight);
		}

		const uint64_t amountCredited = byteBuffer.ReadU64();
		const uint64_t amountDebited = byteBuffer.ReadU64();

		std::optional<Fee> feeOpt = std::nullopt;
		if (byteBuffer.ReadU8() == 1)
		{
			feeOpt = std::make_optional(Fee::Deserialize(byteBuffer));
		}

		// Payment proofs added in version 3
		std::optional<SlatePaymentProof> paymentProofOpt;
		if (walletTxFormat >= 3)
		{
			if (byteBuffer.ReadU8() == 1)
			{
				paymentProofOpt = std::make_optional(SlatePaymentProof::Deserialize(byteBuffer));
			}
		}

		std::optional<Transaction> transactionOpt = std::nullopt;
		if (byteBuffer.GetRemainingSize() > 0)
		{
			transactionOpt = std::make_optional(Transaction::Deserialize(byteBuffer));
		}

		return WalletTx(
			walletTxId, 
			type, 
			std::move(slateIdOpt), 
			std::move(addressOpt),
			std::move(slateMessageOpt), 
			creationTime,
			confirmationTimeOpt, 
			confirmedHeightOpt, 
			amountCredited, 
			amountDebited, 
			feeOpt, 
			std::move(transactionOpt),
			paymentProofOpt
		);
	}

private:
	uint32_t m_walletTxId;
	EWalletTxType m_type; // TODO: Replace with direction & status
	std::optional<uuids::uuid> m_slateIdOpt;
	std::optional<std::string> m_addressOpt;
	std::optional<std::string> m_slateMessageOpt; // TODO: Also include other party's message & signature. Need object for this.
	std::chrono::system_clock::time_point m_creationTime;
	std::optional<std::chrono::system_clock::time_point> m_confirmationTimeOpt;
	std::optional<uint64_t> m_confirmedHeightOpt;
	uint64_t m_amountCredited;
	uint64_t m_amountDebited;
	std::optional<Fee> m_feeOpt;
	std::optional<Transaction> m_transactionOpt;
	std::optional<SlatePaymentProof> m_paymentProofOpt;
};