#pragma once

#include <Crypto/Age/RecipientLine.h>
#include <Crypto/Models/Hash.h>
#include <Crypto/Hasher.h>
#include <Crypto/Models/SecretKey.h>
#include <Crypto/X25519.h>
#include <Core/Exceptions/CryptoException.h>
#include <Core/Serialization/Base64.h>
#include <Common/Util/StringUtil.h>
#include <regex>

namespace age
{
    struct Header
    {
        CBigInteger<32> mac;
        std::vector<RecipientLine> recipients;

        static Header Create(const SecretKey& mac_key, const std::vector<RecipientLine>& recipients)
        {
            std::string encoded_recipients = "";
            for (const RecipientLine& recipient : recipients) {
                encoded_recipients += recipient.Encode();
            }

            std::string encoded_without_mac = StringUtil::Format(
                "age-encryption.org/v1\n{}---",
                encoded_recipients
            );
            std::vector<uint8_t> encoded_u8(encoded_without_mac.cbegin(), encoded_without_mac.cend());

            Hash mac = Hasher::HMAC_SHA256(mac_key.GetVec(), encoded_u8);

            return { mac, recipients };
        }

        void VerifyMac(const SecretKey& mac_key) const
        {
            std::string encoded_recipients = "";
            for (const RecipientLine& recipient : recipients) {
                encoded_recipients += recipient.Encode();
            }

            std::string encoded_without_mac = StringUtil::Format(
                "age-encryption.org/v1\n{}---",
                encoded_recipients
            );
            std::vector<uint8_t> encoded_u8(encoded_without_mac.cbegin(), encoded_without_mac.cend());

            Hash actual = Hasher::HMAC_SHA256(mac_key.GetVec(), encoded_u8);

            if (actual != mac) {
                throw CryptoException("MAC invalid");
            }
        }

        std::string Encode() const noexcept
        {
            std::string encoded_recipients = "";
            for (const RecipientLine& recipient : recipients) {
                encoded_recipients += recipient.Encode();
            }

            return StringUtil::Format(
                "age-encryption.org/v1\n{}--- {}\n",
                encoded_recipients,
                Base64::EncodeUnpadded(mac.GetData())
            );
        }

        static Header Decode(const std::string& encoded)
        {
            std::vector<std::string> lines = StringUtil::Split(encoded, "\n");

            return Decode(lines);
        }

        static Header Decode(const std::vector<std::string>& lines)
        {
            Header header;

            for (auto iter = lines.begin() + 1; iter != lines.end(); iter++) {
                std::string line = *iter;

                std::smatch sm;
                if (std::regex_match(line, sm, std::regex("-> X25519 ([^-\\s]*)"))) {
                    std::vector<uint8_t> pubkey_bytes = Base64::DecodeUnpadded(sm[1]);
                    if (pubkey_bytes.size() != 32) {
                        throw std::exception();
                    }

                    iter++;
                    if (iter == lines.end()) {
                        throw std::exception();
                    }

                    std::vector<uint8_t> file_key_bytes = Base64::DecodeUnpadded(*iter);
                    if (file_key_bytes.size() != 32) {
                        throw std::exception();
                    }

                    RecipientLine recipient;
                    recipient.ephemeral_public_key = Hash(std::move(pubkey_bytes));
                    recipient.encrypted_file_key = CBigInteger<32>(std::move(file_key_bytes));
                    header.recipients.push_back(recipient);
                }
                else if (StringUtil::StartsWith(line, "---")) {
                    std::vector<std::string> parts = StringUtil::Split(line, " ");
                    if (parts.size() != 2) {
                        throw std::exception();
                    }

                    header.mac = CBigInteger<32>(Base64::DecodeUnpadded(parts[1])); // TODO: Verify mac
                    return header;
                }
            }

            throw std::exception();
        }
    };

}