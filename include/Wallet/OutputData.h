#pragma once

#include <Wallet/PrivateExtKey.h>
#include <Wallet/KeyChainPath.h>
#include <Wallet/OutputStatus.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Crypto/Crypto.h>
#include <Crypto/RandomNumberGenerator.h>
#include <optional>

static const uint8_t OUTPUT_DATA_FORMAT = 0;

class OutputData
{
public:
	OutputData(KeyChainPath&& keyChainPath, BlindingFactor&& blindingFactor, TransactionOutput&& output, const uint64_t amount, const EOutputStatus status)
		: m_keyChainPath(std::move(keyChainPath)), m_blindingFactor(std::move(blindingFactor)), m_output(std::move(output)), m_amount(amount), m_status(status), m_mmrIndexOpt(std::nullopt)
	{

	}

	OutputData(KeyChainPath&& keyChainPath, BlindingFactor&& blindingFactor, TransactionOutput&& output, const uint64_t amount, const EOutputStatus status, const std::optional<uint64_t>& mmrIndexOpt)
		: m_keyChainPath(std::move(keyChainPath)), m_blindingFactor(std::move(blindingFactor)), m_output(std::move(output)), m_amount(amount), m_status(status), m_mmrIndexOpt(mmrIndexOpt)
	{

	}

	//
	// Getters
	//
	inline const KeyChainPath& GetKeyChainPath() const { return m_keyChainPath; }
	inline const BlindingFactor& GetBlindingFactor() const { return m_blindingFactor; }
	inline const TransactionOutput& GetOutput() const { return m_output; }
	inline const uint64_t GetAmount() const { return m_amount; }
	inline const EOutputStatus GetStatus() const { return m_status; }
	inline const std::optional<uint64_t>& GetMMRIndex() const { return m_mmrIndexOpt; }

	//
	// Setters
	//
	inline void SetStatus(const EOutputStatus status) { m_status = status; }

	//
	// Operators
	//
	inline bool operator<(const OutputData& other) const { return GetAmount() < other.GetAmount(); }

	//
	// Serialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.Append<uint8_t>(OUTPUT_DATA_FORMAT);
		serializer.AppendVarStr(m_keyChainPath.ToString());
		m_blindingFactor.Serialize(serializer);
		m_output.Serialize(serializer);
		serializer.Append(m_amount);
		serializer.Append((uint8_t)m_status);
		serializer.Append<uint64_t>(m_mmrIndexOpt.value_or(0));
	}

	//
	// Deserialization
	//
	static OutputData Deserialize(ByteBuffer& byteBuffer)
	{
		const uint8_t formatVersion = byteBuffer.ReadU8();
		if (formatVersion != OUTPUT_DATA_FORMAT)
		{
			throw DeserializationException();
		}

		KeyChainPath keyChainPath = KeyChainPath::FromString(byteBuffer.ReadVarStr());
		BlindingFactor blindingFactor = BlindingFactor::Deserialize(byteBuffer);
		TransactionOutput output = TransactionOutput::Deserialize(byteBuffer);
		uint64_t amount = byteBuffer.ReadU64();
		EOutputStatus status = (EOutputStatus)byteBuffer.ReadU8();
		const uint64_t mmrIndex = byteBuffer.ReadU64();

		return OutputData(std::move(keyChainPath), std::move(blindingFactor), std::move(output), amount, status, mmrIndex == 0 ? std::nullopt : std::make_optional<uint64_t>(mmrIndex));
	}

private:
	KeyChainPath m_keyChainPath;
	BlindingFactor m_blindingFactor;
	TransactionOutput m_output;
	uint64_t m_amount;
	EOutputStatus m_status;
	std::optional<uint64_t> m_mmrIndexOpt;
};