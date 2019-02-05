#pragma once

#include <Wallet/PrivateExtKey.h>
#include <Wallet/KeyChainPath.h>
#include <Core/TransactionOutput.h>
#include <Serialization/ByteBuffer.h>
#include <Serialization/Serializer.h>

class OutputData
{
public:
	OutputData(KeyChainPath&& keyChainPath, TransactionOutput&& output, const uint64_t amount)
		: m_keyChainPath(std::move(keyChainPath)), m_output(std::move(output)), m_amount(amount)
	{

	}

	//
	// Getters
	//
	inline const KeyChainPath& GetKeyChainPath() const { return m_keyChainPath; }
	inline const TransactionOutput& GetOutput() const { return m_output; }
	inline const uint64_t GetAmount() const { return m_amount; }

	//
	// Serialization
	//
	void Serialize(Serializer& serializer) const
	{
		serializer.AppendVarStr(m_keyChainPath.ToString());
		m_output.Serialize(serializer);
		serializer.Append(m_amount);
	}

	//
	// Deserialization
	//
	static OutputData Deserialize(ByteBuffer& byteBuffer)
	{
		KeyChainPath keyChainPath = KeyChainPath::FromString(byteBuffer.ReadVarStr());
		TransactionOutput output = TransactionOutput::Deserialize(byteBuffer);
		uint64_t amount = byteBuffer.ReadU64();

		return OutputData(std::move(keyChainPath), std::move(output), amount);
	}

private:
	KeyChainPath m_keyChainPath;
	TransactionOutput m_output;
	uint64_t m_amount;
	// TODO: EOutputStatus
};