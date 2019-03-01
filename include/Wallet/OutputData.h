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
	OutputData(KeyChainPath&& keyChainPath, TransactionOutput&& output, const uint64_t amount, const EOutputStatus status)
		: m_keyChainPath(std::move(keyChainPath)), m_output(std::move(output)), m_amount(amount), m_status(status), m_mmrIndexOpt(std::nullopt)
	{

	}

	OutputData(KeyChainPath&& keyChainPath, TransactionOutput&& output, const uint64_t amount, const EOutputStatus status, const std::optional<uint64_t>& mmrIndexOpt)
		: m_keyChainPath(std::move(keyChainPath)), m_output(std::move(output)), m_amount(amount), m_status(status), m_mmrIndexOpt(mmrIndexOpt)
	{

	}

	//
	// Getters
	//
	inline const KeyChainPath& GetKeyChainPath() const { return m_keyChainPath; }
	inline const TransactionOutput& GetOutput() const { return m_output; }
	inline const uint64_t GetAmount() const { return m_amount; }
	inline const EOutputStatus GetStatus() const { return m_status; }
	inline const std::optional<uint64_t>& GetMMRIndex() const { return m_mmrIndexOpt; }

	//
	// Setters
	//
	inline void SetStatus(const EOutputStatus status) { m_status = status; }

	//
	// Encryption & Serialization
	//
	std::vector<unsigned char> Encrypt(const CBigInteger<32>& masterSeed) const
	{
		const CBigInteger<32> randomNumber = RandomNumberGenerator::GenerateRandom32();
		const CBigInteger<16> iv = CBigInteger<16>(&randomNumber[0]);

		Serializer encryptionSerializer;
		encryptionSerializer.AppendVarStr(m_keyChainPath.ToString());
		m_output.Serialize(encryptionSerializer);
		encryptionSerializer.Append(m_amount);
		encryptionSerializer.Append((uint8_t)m_status);
		encryptionSerializer.Append<uint64_t>(m_mmrIndexOpt.value_or(0));

		const std::vector<unsigned char> encryptedBytes = Crypto::AES256_Encrypt(encryptionSerializer.GetBytes(), masterSeed, iv);

		Serializer serializer;
		serializer.Append<uint8_t>(OUTPUT_DATA_FORMAT);
		serializer.AppendBigInteger(iv);
		serializer.AppendByteVector(encryptedBytes);

		return serializer.GetBytes();
	}

	//
	// Decryption & Deserialization
	//
	static OutputData Decrypt(const CBigInteger<32>& masterSeed, const std::vector<unsigned char>& encrypted)
	{
		ByteBuffer byteBuffer(encrypted);

		const uint8_t formatVersion = byteBuffer.ReadU8();
		if (formatVersion != OUTPUT_DATA_FORMAT)
		{
			throw DeserializationException();
		}

		const CBigInteger<16> iv = byteBuffer.ReadBigInteger<16>();
		const std::vector<unsigned char> encryptedBytes = byteBuffer.ReadVector(byteBuffer.GetRemainingSize());

		const std::vector<unsigned char> decrypted = Crypto::AES256_Decrypt(encryptedBytes, masterSeed, iv);

		KeyChainPath keyChainPath = KeyChainPath::FromString(byteBuffer.ReadVarStr());
		TransactionOutput output = TransactionOutput::Deserialize(byteBuffer);
		uint64_t amount = byteBuffer.ReadU64();
		EOutputStatus status = (EOutputStatus)byteBuffer.ReadU8();
		const uint64_t mmrIndex = byteBuffer.ReadU64();

		return OutputData(std::move(keyChainPath), std::move(output), amount, status, mmrIndex == 0 ? std::nullopt : std::make_optional<uint64_t>(mmrIndex));
	}

private:
	KeyChainPath m_keyChainPath;
	TransactionOutput m_output;
	uint64_t m_amount;
	EOutputStatus m_status;
	std::optional<uint64_t> m_mmrIndexOpt;
};