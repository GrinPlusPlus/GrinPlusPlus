#pragma once

#include <Core/Serialization/Base58.h>
#include <Common/Util/StringUtil.h>
#include <Wallet/Models/Slatepack/SlatepackMessage.h>
#include <Wallet/Models/Slate/Slate.h>

constexpr inline bool IsWhitespace(char c) noexcept {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

class Armor
{
public:
    //
    // If no recipients are provided, slatepack will be plaintext.
    // Otherwise, it will encrypt for each recipient.
    //
    static std::string Pack(
        const SlatepackAddress& sender,
        const Slate& slate,
        const std::vector<SlatepackAddress>& recipients = {})
    {
        Serializer serializer;
        slate.Serialize(serializer);
        SlatepackMessage slatepack(sender, serializer.GetBytes());

        slatepack.m_mode = recipients.empty() ? SlatepackMessage::PLAINTEXT : SlatepackMessage::ENCRYPTED;

        std::vector<uint8_t> serialized = slatepack.Encrypt(recipients);
        std::string base58_encoded = Base58::SimpleEncodeCheck(serialized);

        return StringUtil::Format(
            "{}. {}. {}.",
            HEADER,
            FormatSpacing(base58_encoded),
            FOOTER
        );
    }

    static SlatepackMessage Unpack(const std::string& armored, const x25519_keypair_t& decrypt_keypair)
    {
        std::string cleaned;
        cleaned.reserve(armored.size());

        for (size_t i = 0; i < armored.size(); i++)
        {
            if (!IsWhitespace(armored[i])) {
                cleaned.push_back(armored[i]);
            }
        }

        std::vector<std::string> parts = StringUtil::Split(cleaned, ".");
        if (parts.size() < 3) {
            throw std::exception();
        }

        if (parts[0] != HEADER || parts[2] != FOOTER) {
            throw std::exception();
        }

        std::vector<uint8_t> base58_decoded = Base58::SimpleDecodeCheck(parts[1]);
        return SlatepackMessage::Decrypt(base58_decoded, decrypt_keypair);
    }

private:
    static std::string FormatSpacing(const std::string& data)
    {
        std::string formatted = "";
        for (size_t i = 0; i < data.size(); i++)
        {
            if (i != 0 && i % WORD_LENGTH == 0) {
                if (i % (WORD_LENGTH * WORDS_PER_LINE) == 0) {
                    formatted += "\n";
                } else {
                    formatted += " ";
                }
            }

            formatted += data[i];
        }

        return formatted;
    }

    inline static const std::string HEADER = "BEGINSLATEPACK";
    inline static const std::string FOOTER = "ENDSLATEPACK";

    inline static const int WORD_LENGTH = 15;
    inline static const int WORDS_PER_LINE = 200;
};