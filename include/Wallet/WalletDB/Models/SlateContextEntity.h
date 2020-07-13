#pragma once

#include <Crypto/Crypto.h>
#include <Crypto/ED25519.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <optional>

static const uint8_t SLATE_CONTEXT_FORMAT = 0;

//
// Data Format:
// 32 bytes - blinding factor - encrypted using "blind" key
// 32 bytes - secret nonce - encrypted using "nonce" key
//
// Added in version 1:
// 1 byte - If 1, contains sender proof info.
//
// Sender proof info:
// VarStr - sender address keychain path
// 32 bytes - receiver address// 
//
class SlateContextEntity
{
public:
	SlateContextEntity(SecretKey&& secretKey, SecretKey&& secretNonce, std::vector<OutputDataEntity>&& inputs, std::vector<OutputDataEntity>&& outputs)
		: m_secretKey(std::move(secretKey)), m_secretNonce(std::move(secretNonce)), m_inputs(std::move(inputs)), m_outputs(std::move(outputs)) { }

	SlateContextEntity(const SecretKey& secretKey, const SecretKey& secretNonce, const std::vector<OutputDataEntity>& inputs, const std::vector<OutputDataEntity>& outputs)
		: m_secretKey(secretKey), m_secretNonce(secretNonce), m_inputs(inputs), m_outputs(outputs) { }

	const SecretKey& GetSecretKey() const noexcept { return m_secretKey; }
	PublicKey CalcExcess() const { return Crypto::CalculatePublicKey(m_secretKey); }

	const SecretKey& GetSecretNonce() const noexcept { return m_secretNonce; }
	PublicKey CalcNonce() const { return Crypto::CalculatePublicKey(m_secretNonce); }

	const std::vector<OutputDataEntity>& GetInputs() const noexcept { return m_inputs; }
	const std::vector<OutputDataEntity>& GetOutputs() const noexcept { return m_outputs; }

	void Serialize(Serializer& serializer) const
	{
		serializer.AppendBigInteger<32>(m_secretKey.GetBytes());
		serializer.AppendBigInteger<32>(m_secretNonce.GetBytes());

		serializer.Append<uint64_t>(m_inputs.size());
		for (const OutputDataEntity& input : m_inputs)
		{
			input.Serialize(serializer);
		}

		serializer.Append<uint64_t>(m_outputs.size());
		for (const OutputDataEntity& output : m_outputs)
		{
			output.Serialize(serializer);
		}
	}

	static SlateContextEntity Deserialize(ByteBuffer& byteBuffer)
	{
		SecretKey secretKey = byteBuffer.ReadBigInteger<32>();
		SecretKey secretNonce = byteBuffer.ReadBigInteger<32>();

		std::vector<OutputDataEntity> inputs;
		const size_t numInputs = byteBuffer.ReadU64();
		for (size_t i = 0; i < numInputs; i++)
		{
			inputs.push_back(OutputDataEntity::Deserialize(byteBuffer));
		}

		std::vector<OutputDataEntity> outputs;
		const size_t numOutputs = byteBuffer.ReadU64();
		for (size_t i = 0; i < numOutputs; i++)
		{
			outputs.push_back(OutputDataEntity::Deserialize(byteBuffer));
		}

		return SlateContextEntity(
			std::move(secretKey),
			std::move(secretNonce),
			std::move(inputs),
			std::move(outputs)
		);
	}

private:
	SecretKey m_secretKey;
	SecretKey m_secretNonce;
	std::vector<OutputDataEntity> m_inputs;
	std::vector<OutputDataEntity> m_outputs;
};