#include <Crypto/Age.h>
#include <Crypto/Crypto.h>
#include <Crypto/Curve25519.h>
#include <Crypto/RandomNumberGenerator.h>
#include <chachapoly.h>

static const size_t CHUNK_SIZE = 64 * 1024;
static const size_t TAG_SIZE = 16;
static const size_t ENCRYPTED_CHUNK_SIZE = CHUNK_SIZE + TAG_SIZE;

std::vector<uint8_t> Age::Encrypt(
    const std::vector<x25519_public_key_t>& recipients,
    const std::vector<uint8_t>& payload)
{
    CBigInteger<16> file_key(RandomNumberGenerator().GenerateRandomBytes(16).data());

    SecretKey mac_key = Crypto::HKDF(
        std::nullopt,
        "header",
        file_key.GetData()
    );
    std::vector<age::RecipientLine> recipient_lines = BuildRecipientLines(file_key, recipients);

    age::Header header{ mac_key, recipient_lines };

    CBigInteger<16> nonce(RandomNumberGenerator().GenerateRandomBytes(16).data());
    SecretKey payload_key = Crypto::HKDF(
        std::make_optional(nonce.GetData()),
        "payload",
        file_key.GetData()
    );

    Serializer serializer;
    serializer.AppendFixedStr(header.Encode());
    serializer.AppendBigInteger(nonce);
    StreamEncrypt(serializer, payload_key, payload);

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

        std::vector<uint8_t> salt;
        salt.insert(salt.end(), ephemeral_keypair.pubkey.cbegin(), ephemeral_keypair.pubkey.cend());
        salt.insert(salt.end(), recipient.cbegin(), recipient.cend());
        SecretKey enc_key = Crypto::HKDF(
            std::make_optional(std::move(salt)),
            "age-encryption.org/v1/X25519",
            shared_secret.GetVec()
        );

        chachapoly_ctx context;
        const int init_result = chachapoly_init(&context, enc_key.data(), (int)enc_key.size());
        if (init_result != 0) {
            throw std::exception();
        }

        CBigInteger<12> nonce = CBigInteger<12>::ValueOf(0);
        SecretKey encrypted;
        const int crypt_result = chachapoly_crypt(
            &context,
            nonce.data(),
            nullptr,
            0,
            file_key.data(),
            (int)file_key.size(),
            encrypted.data(),
            nullptr,
            0,
            1
        );
        if (init_result != 0) {
            throw std::exception();
        }

        recipientLines.push_back(age::RecipientLine{ ephemeral_keypair.pubkey, encrypted });
    }

    return recipientLines;
}

void Age::StreamEncrypt(Serializer& serializer, const SecretKey& key, const std::vector<uint8_t>& data)
{
    chachapoly_ctx context;
    const int init_result = chachapoly_init(&context, key.data(), (int)key.size());
    if (init_result != 0) {
        throw std::exception();
    }

    CBigInteger<11> counter;

    size_t index = 0;
    while (index < data.size())
    {
        size_t to_write = (std::min)(CHUNK_SIZE, (data.size() - index));
        size_t next_index = index + to_write;

        std::vector<uint8_t> nonce = counter.GetData();
        if (next_index == data.size()) {
            nonce.push_back(1);
        } else {
            nonce.push_back(0);
        }

        std::vector<uint8_t> encrypted_chunk(to_write, 0);
        const int crypt_result = chachapoly_crypt(
            &context,
            nonce.data(),
            nullptr,
            0,
            data.data() + index,
            (int)to_write,
            encrypted_chunk.data(),
            nullptr,
            0,
            1
        );
        if (init_result != 0) {
            throw std::exception();
        }

        serializer.AppendByteVector(encrypted_chunk);

        index = next_index;
        counter++;
    }
}