#pragma once

#include <Crypto/Models/SecretKey.h>
#include <Core/Exceptions/CryptoException.h>
#include <chachapoly.h>

class ChaChaPoly
{
    static inline const size_t CHUNK_SIZE = 64 * 1024;
    static inline const size_t TAG_SIZE = 16;
    static inline const size_t ENCRYPTED_CHUNK_SIZE = CHUNK_SIZE + TAG_SIZE;

public:
    static ChaChaPoly Init(const SecretKey& enc_key)
    {
        chachapoly_ctx context;
        const int init_result = chachapoly_init(&context, enc_key.data(), 256);
        if (init_result != 0) {
            throw CryptoException("Failed to Init");
        }

        return ChaChaPoly{ context };
    }

    std::vector<uint8_t> Encrypt(const CBigInteger<12>& nonce, const std::vector<uint8_t>& decrypted)
    {
        std::vector<uint8_t> encrypted(decrypted.size(), 0);
        std::vector<uint8_t> tag(TAG_SIZE, 0);
        const int crypt_result = chachapoly_crypt(
            &m_context,
            nonce.data(),
            nullptr,
            0,
            decrypted.data(),
            (int)decrypted.size(),
            encrypted.data(),
            tag.data(),
            (int)tag.size(),
            1
        );
        if (crypt_result != 0) {
            throw CryptoException("Failed to encrypt");
        }

        encrypted.insert(encrypted.end(), tag.cbegin(), tag.cend());
        return encrypted;
    }

    std::vector<uint8_t> StreamEncrypt(const std::vector<uint8_t>& data)
    {
        std::vector<uint8_t> encrypted;

        CBigInteger<11> counter;

        size_t index = 0;
        while (index < data.size())
        {
            size_t to_write = (std::min)(CHUNK_SIZE, (data.size() - index));
            size_t next_index = index + to_write;

            std::vector<uint8_t> nonce = counter.GetData();
            nonce.push_back(next_index == data.size() ? 1 : 0);

            std::vector<uint8_t> encrypted_chunk = Encrypt(
                CBigInteger<12>{ nonce },
                std::vector<uint8_t>{ data.begin() + index, data.begin() + index + to_write }
            );

            encrypted.insert(encrypted.end(), encrypted_chunk.cbegin(), encrypted_chunk.cend());

            index = next_index;
            counter++;
        }

        return encrypted;
    }

    std::vector<uint8_t> Decrypt(const CBigInteger<12>& nonce, const std::vector<uint8_t>& encrypted)
    {
        assert(encrypted.size() > TAG_SIZE);

        const size_t encrypted_len = encrypted.size() - TAG_SIZE;
        std::vector<uint8_t> ciphertext(encrypted.cbegin(), encrypted.cbegin() + encrypted_len);
        std::vector<uint8_t> tag(encrypted.cbegin() + encrypted_len, encrypted.cend());

        std::vector<uint8_t> decrypted(encrypted_len, 0);
        const int crypt_result = chachapoly_crypt(
            &m_context,
            nonce.data(),
            nullptr,
            0,
            ciphertext.data(),
            (int)ciphertext.size(),
            decrypted.data(),
            tag.data(),
            (int)tag.size(),
            0
        );
        if (crypt_result != 0) {
            throw CryptoException("Failed to decrypt");
        }

        return decrypted;
    }

    std::vector<uint8_t> StreamDecrypt(const std::vector<uint8_t>& encrypted)
    {
        std::vector<uint8_t> decrypted;

        CBigInteger<11> counter;

        size_t index = 0;
        while (index < encrypted.size())
        {
            size_t to_read = (std::min)(ENCRYPTED_CHUNK_SIZE, (encrypted.size() - index));
            size_t next_index = index + to_read;

            std::vector<uint8_t> nonce = counter.GetData();
            nonce.push_back(next_index == encrypted.size() ? 1 : 0);

            std::vector<uint8_t> decrypted_chunk = Decrypt(
                CBigInteger<12>{ nonce },
                std::vector<uint8_t>{ encrypted.begin() + index, encrypted.begin() + index + to_read }
            );

            decrypted.insert(decrypted.end(), decrypted_chunk.cbegin(), decrypted_chunk.cend());

            index = next_index;
            counter++;
        }

        return decrypted;
    }

private:
    ChaChaPoly(const chachapoly_ctx& context)
        : m_context(context) { }

    chachapoly_ctx m_context;
};