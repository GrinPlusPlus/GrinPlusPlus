#pragma once

#include <Wallet/WalletTxType.h>
#include <Core/Models/Transaction.h>
#include <Common/Util/TimeUtil.h>
#include <uuid.h>
#include <chrono>
#include <optional>

static const uint8_t WALLET_TX_DATA_FORMAT = 0;

class WalletTx
{
public:
	// TODO: Save Slate messages
	WalletTx(
		const uint32_t walletTxId,
		const EWalletTxType type,
		std::optional<uuids::uuid>&& slateIdOpt,
		const std::chrono::system_clock::time_point& creationTime,
		const std::optional<std::chrono::system_clock::time_point>& confirmationTimeOpt,
		const uint32_t numInputs,
		const uint32_t numOutputs,
		const uint64_t amountCredited,
		const uint64_t amountDebited,
		const std::optional<uint64_t>& feeOpt,
		std::optional<Transaction>&& transactionOpt
	)	
		: m_walletTxId(walletTxId),
		m_type(type),
		m_slateIdOpt(std::move(slateIdOpt)),
		m_creationTime(creationTime),
		m_confirmationTimeOpt(confirmationTimeOpt),
		m_numInputs(numInputs),
		m_numOutputs(numOutputs),
		m_amountCredited(amountCredited),
		m_amountDebited(amountDebited),
		m_feeOpt(feeOpt),
		m_transactionOpt(std::move(transactionOpt))
	{

	}

	inline uint32_t GetId() const { return m_walletTxId; }
	inline EWalletTxType GetType() const { return m_type; }
	inline const std::optional<uuids::uuid>& GetSlateId() const { return m_slateIdOpt; }
	inline const std::chrono::system_clock::time_point& GetCreationTime() const { return m_creationTime; }
	inline const std::optional<std::chrono::system_clock::time_point>& GetConfirmationTime() const { return m_confirmationTimeOpt; }
	inline uint32_t GetNumInputs() const { return m_numInputs; }
	inline uint32_t GetNumOutputs() const { return m_numOutputs; }
	inline uint64_t GetAmountCredited() const { return m_amountCredited; }
	inline uint64_t GetAmountDebited() const { return m_amountDebited; }
	inline std::optional<uint64_t> GetFee() const { return m_feeOpt; }
	inline const std::optional<Transaction>& GetTransaction() const { return m_transactionOpt; }

	inline void SetType(const EWalletTxType type) { m_type = type; }
	inline void SetTransaction(const Transaction& transaction) { m_transactionOpt = std::make_optional<Transaction>(Transaction(transaction)); }

	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint8_t>(WALLET_TX_DATA_FORMAT);
		serializer.Append<uint32_t>(m_walletTxId);
		serializer.Append<uint8_t>((uint8_t)m_type);
		serializer.AppendVarStr(m_slateIdOpt.has_value() ? uuids::to_string(m_slateIdOpt.value()) : "");
		serializer.Append<int64_t>(TimeUtil::ToInt64(m_creationTime));
		serializer.Append<int64_t>((int64_t)(m_confirmationTimeOpt.has_value() ? TimeUtil::ToInt64(m_confirmationTimeOpt.value()) : 0));
		serializer.Append<uint32_t>(m_numInputs);
		serializer.Append<uint32_t>(m_numOutputs);
		serializer.Append<uint64_t>(m_amountCredited);
		serializer.Append<uint64_t>(m_amountDebited);

		if (m_feeOpt.has_value())
		{
			serializer.Append<uint8_t>(1);
			serializer.Append<uint64_t>(m_feeOpt.value());
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
		if (byteBuffer.ReadU8() != WALLET_TX_DATA_FORMAT)
		{
			throw DeserializationException();
		}

		const uint32_t walletTxId = byteBuffer.ReadU32();
		const EWalletTxType type = (EWalletTxType)byteBuffer.ReadU8();

		std::optional<uuids::uuid> slateIdOpt = std::nullopt;
		std::string slateIdStr = byteBuffer.ReadVarStr();
		if (!slateIdStr.empty())
		{
			slateIdOpt = uuids::uuid::from_string(slateIdStr);
		}

		std::chrono::system_clock::time_point creationTime = TimeUtil::ToTimePoint(byteBuffer.Read64());

		std::optional<std::chrono::system_clock::time_point> confirmationTimeOpt = std::nullopt;
		const int64_t confirmationTimeMillis = byteBuffer.Read64();
		if (confirmationTimeMillis != 0)
		{
			confirmationTimeOpt = std::make_optional<std::chrono::system_clock::time_point>(TimeUtil::ToTimePoint(confirmationTimeMillis));
		}

		const uint32_t numInputs = byteBuffer.ReadU32();
		const uint32_t numOutputs = byteBuffer.ReadU32();
		const uint64_t amountCredited = byteBuffer.ReadU64();
		const uint64_t amountDebited = byteBuffer.ReadU64();

		std::optional<uint64_t> feeOpt = std::nullopt;
		if (byteBuffer.ReadU8() == 1)
		{
			feeOpt = std::make_optional<uint64_t>(byteBuffer.ReadU64());
		}

		std::optional<Transaction> transactionOpt = std::nullopt;
		if (byteBuffer.GetRemainingSize() > 0)
		{
			transactionOpt = std::make_optional<Transaction>(Transaction::Deserialize(byteBuffer));
		}

		return WalletTx(walletTxId, type, std::move(slateIdOpt), creationTime, confirmationTimeOpt, numInputs, numOutputs, amountCredited, amountDebited, feeOpt, std::move(transactionOpt));
	}

private:
	uint32_t m_walletTxId;
	EWalletTxType m_type; // TODO: Replace with direction & status
	std::optional<uuids::uuid> m_slateIdOpt;
	std::chrono::system_clock::time_point m_creationTime;
	std::optional<std::chrono::system_clock::time_point> m_confirmationTimeOpt;
	uint32_t m_numInputs;
	uint32_t m_numOutputs;
	uint64_t m_amountCredited;
	uint64_t m_amountDebited;
	std::optional<uint64_t> m_feeOpt;
	std::optional<Transaction> m_transactionOpt;
};