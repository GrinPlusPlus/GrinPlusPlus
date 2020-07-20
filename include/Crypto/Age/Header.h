#pragma once

#include <Crypto/Age/RecipientLine.h>
#include <Crypto/Hash.h>
#include <Crypto/SecretKey.h>
#include <Crypto/X25519.h>
#include <Core/Serialization/Base64.h>
#include <Common/Util/StringUtil.h>
#include <regex>

namespace age
{
    struct Header
    {
        CBigInteger<32> mac;
        std::vector<RecipientLine> recipients;

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