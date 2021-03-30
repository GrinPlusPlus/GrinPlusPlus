#pragma once

#include <Wallet/PrivateExtKey.h>
#include <Wallet/KeyChainPath.h>
#include <Wallet/Enums/OutputStatus.h>
#include <Core/Traits/Printable.h>
#include <Core/Traits/Serializable.h>
#include <Core/Models/TransactionOutput.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Crypto/Models/BlindingFactor.h>
#include <Crypto/Models/SecretKey.h>
#include <optional>

static const uint8_t OUTPUT_DATA_FORMAT = 1;

class OutputDataEntity : public Traits::IPrintable, public Traits::ISerializable
{
public:
    //
    // Constructors
    //
    OutputDataEntity(
        KeyChainPath keyChainPath,
        SecretKey blindingFactor,
        TransactionOutput output,
        const uint64_t amount,
        const EOutputStatus status,
        std::optional<uint64_t> mmr_index,
        std::optional<uint64_t> block_height,
        std::optional<uint32_t> walletTxIdOpt,
        std::vector<std::string> labels
    )
        : m_keyChainPath(std::move(keyChainPath)),
        m_blindingFactor(std::move(blindingFactor)),
        m_output(std::move(output)),
        m_amount(amount),
        m_status(status),
        m_mmrIndexOpt(mmr_index),
        m_blockHeightOpt(block_height),
        m_walletTxIdOpt(walletTxIdOpt),
        m_labels(std::move(labels))
    {

    }

    //
    // Destructor
    //
    virtual ~OutputDataEntity() = default;

    //
    // Getters
    //
    const KeyChainPath& GetKeyChainPath() const { return m_keyChainPath; }
    BlindingFactor GetBlindingFactor() const { return BlindingFactor(m_blindingFactor.GetBytes()); }
    const TransactionOutput& GetOutput() const { return m_output; }
    uint64_t GetAmount() const { return m_amount; }
    EOutputStatus GetStatus() const { return m_status; }
    const std::optional<uint64_t>& GetMMRIndex() const { return m_mmrIndexOpt; }
    const std::optional<uint64_t>& GetBlockHeight() const { return m_blockHeightOpt; }
    const std::optional<uint32_t>& GetWalletTxId() const { return m_walletTxIdOpt; }
    const std::vector<std::string>& GetLabels() const { return m_labels; }
    const Commitment& GetCommitment() const { return m_output.GetCommitment(); }
    const RangeProof& GetRangeProof() const { return m_output.GetRangeProof(); }
    EOutputFeatures GetFeatures() const { return m_output.GetFeatures(); }

    //
    // Setters
    //
    void SetStatus(const EOutputStatus status) { m_status = status; }
    void SetBlockHeight(const uint64_t blockHeight) { m_blockHeightOpt = std::make_optional(blockHeight); }
    void SetWalletTxId(const uint32_t walletTxId) { m_walletTxIdOpt = std::make_optional(walletTxId); }

    //
    // Operators
    //
    bool operator<(const OutputDataEntity& other) const { return GetAmount() < other.GetAmount(); }

    //
    // Serialization
    //
    void Serialize(Serializer& serializer) const final
    {
        serializer.Append<uint8_t>(OUTPUT_DATA_FORMAT);
        serializer.AppendVarStr(m_keyChainPath.Format());
        m_blindingFactor.Serialize(serializer);
        m_output.Serialize(serializer);
        serializer.Append(m_amount);
        serializer.Append((uint8_t)m_status);
        serializer.Append<uint64_t>(m_mmrIndexOpt.value_or(0));
        serializer.Append<uint64_t>(m_blockHeightOpt.value_or(0));
        serializer.AppendVarStr(m_labels.empty() ? "" : m_labels.front());

        if (m_walletTxIdOpt.has_value()) {
            serializer.Append<uint32_t>(m_walletTxIdOpt.value());
        }
    }

    //
    // Deserialization
    //
    static OutputDataEntity Deserialize(ByteBuffer& byteBuffer)
    {
        const uint8_t formatVersion = byteBuffer.ReadU8();
        if (formatVersion > OUTPUT_DATA_FORMAT) {
            throw DESERIALIZATION_EXCEPTION_F(
                "Expected format <= {}, but was {}",
                OUTPUT_DATA_FORMAT,
                formatVersion
            );
        }

        KeyChainPath keyChainPath = KeyChainPath::FromString(byteBuffer.ReadVarStr());
        SecretKey blindingFactor = SecretKey::Deserialize(byteBuffer);
        TransactionOutput output = TransactionOutput::Deserialize(byteBuffer);
        uint64_t amount = byteBuffer.ReadU64();
        EOutputStatus status = (EOutputStatus)byteBuffer.ReadU8();

        const uint64_t mmrIndex = byteBuffer.ReadU64();
        const std::optional<uint64_t> mmrIndexOpt = mmrIndex == 0 ? std::nullopt : std::make_optional(mmrIndex);

        const uint64_t blockHeight = byteBuffer.GetRemainingSize() != 0 ? byteBuffer.ReadU64() : 0;
        const std::optional<uint64_t> blockHeightOpt = blockHeight == 0 ? std::nullopt : std::make_optional(blockHeight);

        std::vector<std::string> labels;
        if (formatVersion >= 1) {
            std::string label = byteBuffer.ReadVarStr();
            if (!label.empty()) {
                labels.push_back(label);
            }
        }

        std::optional<uint32_t> walletTxIdOpt = std::nullopt;
        if (byteBuffer.GetRemainingSize() != 0) {
            walletTxIdOpt = std::make_optional(byteBuffer.ReadU32());
        }

        return OutputDataEntity(
            std::move(keyChainPath),
            std::move(blindingFactor),
            std::move(output),
            amount,
            status,
            mmrIndexOpt,
            blockHeightOpt,
            walletTxIdOpt,
            labels
        );
    }

    //Json::Value ToJSON() const
    //{

    //}

    //static OutputDataEntity FromJSON(const Json::Value& json)
    //{

    //}

    //
    // Traits
    //
    std::string Format() const final { return m_output.GetCommitment().Format(); }

private:
    KeyChainPath m_keyChainPath;
    SecretKey m_blindingFactor;
    TransactionOutput m_output;
    uint64_t m_amount;
    EOutputStatus m_status;
    std::optional<uint64_t> m_mmrIndexOpt;
    std::optional<uint64_t> m_blockHeightOpt;
    std::optional<uint32_t> m_walletTxIdOpt;
    std::vector<std::string> m_labels;
};