#include <Crypto/Age.h>
#include <Crypto/Curve25519.h>
#include <Crypto/CSPRNG.h>
#include <Crypto/ChaChaPoly.h>
#include <Crypto/KDF.h>

std::vector<uint8_t> Age::Encrypt(
    const std::vector<x25519_public_key_t>& recipients,
    const std::vector<uint8_t>& payload)
{
    CBigInteger<16> file_key(CSPRNG().GenerateRandomBytes(16).data());

    SecretKey mac_key = KDF::HKDF(
        std::nullopt,
        "header",
        file_key.GetData()
    );
    std::vector<age::RecipientLine> recipient_lines = BuildRecipientLines(file_key, recipients);

    age::Header header = age::Header::Create(mac_key, recipient_lines);

    CBigInteger<16> nonce(CSPRNG().GenerateRandomBytes(16).data());
    SecretKey payload_key = KDF::HKDF(
        std::make_optional(nonce.GetData()),
        "payload",
        file_key.GetData()
    );

    Serializer serializer;
    serializer.AppendStr(header.Encode());
    serializer.AppendBigInteger(nonce);

    std::vector<uint8_t> encrypted_payload = ChaChaPoly::Init(payload_key).StreamEncrypt(payload);
    serializer.AppendByteVector(encrypted_payload);

    return serializer.GetBytes();
}

std::vector<age::RecipientLine> Age::BuildRecipientLines(
    const CBigInteger<16>& file_key,
    const std::vector<x25519_public_key_t>& recipients)
{
    std::vector<age::RecipientLine> recipientLines;

    for (const x25519_public_key_t& recipient : recipients) {
        x25519_keypair_t ephemeral_keypair = X25519::GenerateEphemeralKeypair();
        SecretKey shared_secret = X25519::ECDH(ephemeral_keypair.seckey, recipient);

        SecretKey enc_key = age::RecipientLine::CalcEncryptionKey(
            shared_secret,
            ephemeral_keypair.pubkey,
            recipient
        );

        std::vector<uint8_t> encrypted = ChaChaPoly::Init(enc_key).Encrypt(
            CBigInteger<12>::ValueOf(0),
            file_key.GetData()
        );

        recipientLines.push_back(age::RecipientLine{ ephemeral_keypair.pubkey, CBigInteger<32>{ encrypted.data() } });
    }

    return recipientLines;
}

std::vector<uint8_t> Age::Decrypt(
    const x25519_keypair_t& decrypt_keypair,
    const std::vector<uint8_t>& payload)
{
    std::vector<std::string> encoded_header_lines;
    ByteBuffer deserializer(payload);
    std::string encoded_header_line;
    while (true)
    {
        char c = (char)deserializer.ReadU8();
        if (c == '\n') {
            encoded_header_lines.push_back(encoded_header_line);
            if (StringUtil::StartsWith(encoded_header_line, "---")) {
                break;
            }

            encoded_header_line = "";
        } else {
            encoded_header_line += c;
        }
    }

    CBigInteger<16> file_key_nonce = deserializer.ReadBigInteger<16>();
    std::vector<uint8_t> encrypted_payload = deserializer.ReadRemainingBytes();

    age::Header header = age::Header::Decode(encoded_header_lines);

    for (const age::RecipientLine& recipient_line : header.recipients)
    {
        std::vector<uint8_t> decrypted = TryDecrypt(
            decrypt_keypair,
            recipient_line,
            file_key_nonce,
            header,
            encrypted_payload
        );
        if (!decrypted.empty()) {
            return decrypted;
        }
    }

    throw std::exception();
}

std::vector<uint8_t> Age::TryDecrypt(
    const x25519_keypair_t& decrypt_keypair,
    const age::RecipientLine& recipient_line,
    const CBigInteger<16>& file_key_nonce,
    const age::Header& /*header*/,
    const std::vector<uint8_t>& encrypted_payload)
{
    try
    {
        SecretKey shared_secret = X25519::ECDH(decrypt_keypair.seckey, recipient_line.ephemeral_public_key);

        SecretKey enc_key = age::RecipientLine::CalcEncryptionKey(
            shared_secret,
            recipient_line.ephemeral_public_key,
            decrypt_keypair.pubkey
        );

        std::vector<uint8_t> decrypted_file_key = ChaChaPoly::Init(enc_key).Decrypt(
            CBigInteger<12>::ValueOf(0),
            recipient_line.encrypted_file_key.GetData()
        );

        SecretKey mac_key = KDF::HKDF(
            std::nullopt,
            "header",
            decrypted_file_key
        );
        //header.VerifyMac(mac_key);

        SecretKey payload_key = KDF::HKDF(
            std::make_optional(file_key_nonce.GetData()),
            "payload",
            decrypted_file_key
        );

        return ChaChaPoly::Init(payload_key).StreamDecrypt(encrypted_payload);
    }
    catch (const std::exception&)
    {

    }

    return {};
}