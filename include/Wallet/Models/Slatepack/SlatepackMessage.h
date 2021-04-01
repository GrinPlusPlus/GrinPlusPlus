#pragma once

#include <Wallet/Models/Slatepack/SlatepackAddress.h>
#include <Core/Util/JsonUtil.h>
#include <Core/Serialization/Base64.h>
#include <Core/Serialization/ByteBuffer.h>
#include <Core/Serialization/Serializer.h>
#include <Crypto/Age.h>
#include <cstdint>
#include <optional>

#undef major
#undef minor

struct SlatepackMessage
{
    struct Version
    {
        Version() : major(1), minor(0) { } // TODO: Determine default values

        uint8_t major;
        uint8_t minor;

        void Serialize(Serializer& serializer) const noexcept
        {
            serializer.Append<uint8_t>(major);
            serializer.Append<uint8_t>(minor);
        }

        static Version Deserialize(ByteBuffer& byteBuffer)
        {
            Version version;
            version.major = byteBuffer.ReadU8();
            version.minor = byteBuffer.ReadU8();
            return version;
        }

        std::string ToString() const noexcept
        {
            return StringUtil::Format("{}:{}", major, minor);
        }
    };

    struct Metadata
    {
        Metadata(const SlatepackAddress& sender_, const std::vector<SlatepackAddress>& recipients_)
            : sender(sender_), recipients(recipients_) { }

        SlatepackAddress sender;
        std::vector<SlatepackAddress> recipients;

        void Serialize(Serializer& serializer) const noexcept
        {
            uint16_t opt_flags = 0x01;
            if (!recipients.empty()) {
                opt_flags |= 0x02;
            }

            Serializer tempSerializer;
            tempSerializer.Append<uint16_t>(opt_flags);
            sender.Serialize(tempSerializer);

            if (!recipients.empty()) {
                tempSerializer.Append<uint16_t>((uint16_t)recipients.size());

                for (const SlatepackAddress& recipient : recipients)
                {
                    recipient.Serialize(tempSerializer);
                }
            }

            serializer.Append(tempSerializer.GetBytes(), ESerializeLength::U32);
        }

        static Metadata Deserialize(ByteBuffer& byteBuffer)
        {
            uint32_t size = byteBuffer.ReadU32();
            ByteBuffer innerBuffer(byteBuffer.ReadVector(size));

            uint16_t opt_flags = innerBuffer.ReadU16();

            SlatepackAddress sender;
            if ((opt_flags & 0x01) == 0x01) {
                sender = SlatepackAddress::Deserialize(innerBuffer);
            }

            std::vector<SlatepackAddress> recipients;
            if ((opt_flags & 0x02) == 0x02) {
                uint16_t num_recipients = innerBuffer.ReadU16();
                for (uint16_t i = 0; i < num_recipients; i++)
                {
                    recipients.push_back(SlatepackAddress::Deserialize(innerBuffer));
                }
            }

            return Metadata{ sender, recipients };
        }
    };

    enum EMode : uint8_t
    {
        PLAINTEXT,
        ENCRYPTED
    };

    SlatepackMessage()
        : m_version(), m_mode(ENCRYPTED), m_sender(), m_recipients(), m_payload() { }
    SlatepackMessage(const SlatepackAddress& sender, const std::vector<uint8_t>& payload)
        : m_version(), m_mode(ENCRYPTED), m_sender(sender), m_recipients(), m_payload(payload) { }

    Version m_version;
    EMode m_mode;
    SlatepackAddress m_sender;
    std::vector<SlatepackAddress> m_recipients;

    // Can be encrypted or plaintext
    std::vector<uint8_t> m_payload;

    std::vector<uint8_t> Encrypt(const std::vector<SlatepackAddress>& recipients) const
    {
        Serializer serializer;
        m_version.Serialize(serializer);
        serializer.Append<uint8_t>((uint8_t)m_mode);

        uint16_t opt_flags = 0x00;
        if (m_mode == EMode::PLAINTEXT && !m_sender.IsNull()) {
            opt_flags |= 0x01; // Sender
        }

        serializer.Append<uint16_t>(opt_flags);

        {
            Serializer optSerializer;
            if ((opt_flags & 0x01) == 0x01) {
                m_sender.Serialize(optSerializer);
            }

            serializer.Append(optSerializer.GetBytes(), ESerializeLength::U32);
        }

        if (m_mode == EMode::PLAINTEXT) {
            serializer.Append(m_payload, ESerializeLength::U64);
        } else {
            serializer.Append(EncryptPayload(recipients), ESerializeLength::U64);
        }

        return serializer.GetBytes();
    }

    static SlatepackMessage Decrypt(const std::vector<uint8_t>& encrypted, const x25519_keypair_t& decrypt_keypair)
    {
        ByteBuffer byteBuffer(encrypted);

        SlatepackMessage slatepack{ };
        slatepack.m_version = Version::Deserialize(byteBuffer);
        slatepack.m_mode = (EMode)byteBuffer.ReadU8();

        uint16_t opt_flags = byteBuffer.ReadU16();
        uint32_t opt_fields_len = byteBuffer.ReadU32();
        
        ByteBuffer opt_fields_buffer = byteBuffer.ReadVector(opt_fields_len);
        if ((opt_flags & 0x01) == 0x01) {
            slatepack.m_sender = SlatepackAddress::Deserialize(opt_fields_buffer);
        }

        uint64_t payload_size = byteBuffer.ReadU64();
        slatepack.m_payload = byteBuffer.ReadVector(payload_size);

        if (slatepack.m_mode == EMode::ENCRYPTED) {
            slatepack.DecryptPayload(decrypt_keypair);
        }

        return slatepack;
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

private:
    std::vector<uint8_t> EncryptPayload(const std::vector<SlatepackAddress>& recipients) const
    {
        Serializer innerSerializer;

        uint16_t flags = 0x01;
        if (!m_recipients.empty()) {
            flags |= 0x02;
        }

        innerSerializer.Append<uint16_t>(flags);

        m_sender.Serialize(innerSerializer);

        if (!m_recipients.empty()) {
            assert(m_recipients.size() <= (size_t)UINT16_MAX);
            innerSerializer.Append<uint16_t>((uint16_t)m_recipients.size());

            for (const SlatepackAddress& recipient : m_recipients) {
                recipient.Serialize(innerSerializer);
            }
        }

        Serializer serializer;
        serializer.AppendByteVector(innerSerializer.GetBytes(), ESerializeLength::U32);
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

    void DecryptPayload(const x25519_keypair_t& decrypt_keypair)
    {
        std::vector<uint8_t> decrypted = Age::Decrypt(decrypt_keypair, m_payload);
        ByteBuffer buffer(std::move(decrypted));

        Metadata metadata = Metadata::Deserialize(buffer);
        if (m_mode == EMode::ENCRYPTED) {
            m_sender = metadata.sender;
            m_recipients = metadata.recipients;
        }

        m_payload = buffer.ReadRemainingBytes();
    }
};