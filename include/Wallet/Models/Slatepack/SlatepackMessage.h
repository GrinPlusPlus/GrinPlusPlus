#pragma once

#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Serialization/Base64.h>
#include <Crypto/Age.h>
#include <cstdint>
#include <optional>

struct SlatepackMessage
{
    struct Version
    {
        Version() : major(1), minor(0) { } // TODO: Determine values

        uint8_t major;
        uint8_t minor;

        std::string ToString() const noexcept
        {
            return StringUtil::Format("{}:{}", major, minor);
        }
    };

    enum EMode : uint8_t
    {
        PLAINTEXT,
        ENCRYPTED
    };

    SlatepackMessage() = default;

    Version m_version;
    EMode m_mode;
    SlatepackAddress m_sender;
    std::vector<SlatepackAddress> m_recipients;

    // Can be encrypted or plaintext
    std::vector<uint8_t> m_payload;

    std::vector<uint8_t> EncryptPayload(const std::vector<SlatepackAddress>& recipients) const noexcept
    {
        Serializer innerSerializer;
        m_sender.Serialize(innerSerializer);

        uint16_t flags = 0x01;
        if (!m_recipients.empty()) {
            flags |= 0x02;
        }

        innerSerializer.Append<uint16_t>(flags);

        if (!m_recipients.empty()) {
            assert(m_recipients.size() <= (size_t)UINT16_MAX);
            innerSerializer.Append<uint16_t>((uint16_t)m_recipients.size());

            for (const SlatepackAddress& recipient : m_recipients) {
                recipient.Serialize(innerSerializer);
            }
        }

        Serializer serializer;
        serializer.Append<uint32_t>((uint32_t)innerSerializer.size());
        serializer.AppendByteVector(innerSerializer.GetBytes());
        serializer.AppendByteVector(m_payload);

        std::vector<uint8_t> to_encrypt = serializer.GetBytes();

        std::vector<x25519_public_key_t> recipient_pubkeys;
        std::transform(
            recipients.cbegin(), recipients.cend(),
            std::back_inserter(recipient_pubkeys),
            [](const SlatepackAddress& address) { return address.ToX25519PubKey(); }
        );

        return Age::Encrypt(recipient_pubkeys, to_encrypt);
    }

    Json::Value ToJSON() const noexcept
    {
        Json::Value json;
        json["version"] = m_version.ToString();
        json["mode"] = (uint8_t)m_mode;

        if (m_mode == EMode::PLAINTEXT) {
            json["sender"] = m_sender.ToString();
        } else {
            Json::Value encJson;
            encJson["sender"] = m_sender.ToString();

            if (!m_recipients.empty()) {
                Json::Value recipientsJson(Json::arrayValue);
                for (const SlatepackAddress& recipient : m_recipients) {
                    recipientsJson.append(recipient.ToString());
                }
                json["recipients"] = recipientsJson;
            }

            json["encrypted_meta"] = encJson;
        }

        if (!m_payload.empty()) {
            json["payload"] = Base64::EncodeUnpadded(m_payload);
        }

        return json;
    }
};