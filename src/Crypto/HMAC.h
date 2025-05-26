#pragma once

#include <bitcoin/sha256.h>
#include <bitcoin/sha512.h>

#include <cstdint>
#include <cstdlib>

/** A hasher class for HMAC-SHA-256. */
class CHMAC_SHA256
{
private:
    CSHA256 outer;
    CSHA256 inner;

public:
    static const size_t OUTPUT_SIZE = 32;

    CHMAC_SHA256(const unsigned char* key, size_t keylen)
    {
        unsigned char rkey[64];
        if (keylen <= 64) {
            memcpy(rkey, key, keylen);
            memset(rkey + keylen, 0, 64 - keylen);
        }
        else {
            CSHA256().Write(key, keylen).Finalize(rkey);
            memset(rkey + 32, 0, 32);
        }

        for (int n = 0; n < 64; n++)
            rkey[n] ^= 0x5c;
        outer.Write(rkey, 64);

        for (int n = 0; n < 64; n++)
            rkey[n] ^= 0x5c ^ 0x36;
        inner.Write(rkey, 64);
    }
    CHMAC_SHA256& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE])
    {
        unsigned char temp[32];
        inner.Finalize(temp);
        outer.Write(temp, 32).Finalize(hash);
    }
};

/** A hasher class for HMAC-SHA-512. */
class CHMAC_SHA512
{
private:
    CSHA512 outer;
    CSHA512 inner;

public:
    static const size_t OUTPUT_SIZE = 64;

    CHMAC_SHA512(const unsigned char* key, size_t keylen)
    {
        unsigned char rkey[128];
        if (keylen <= 128) {
            memcpy(rkey, key, keylen);
            memset(rkey + keylen, 0, 128 - keylen);
        }
        else {
            CSHA512().Write(key, keylen).Finalize(rkey);
            memset(rkey + 64, 0, 64);
        }

        for (int n = 0; n < 128; n++)
            rkey[n] ^= 0x5c;
        outer.Write(rkey, 128);

        for (int n = 0; n < 128; n++)
            rkey[n] ^= 0x5c ^ 0x36;
        inner.Write(rkey, 128);
    }
    CHMAC_SHA512& Write(const unsigned char* data, size_t len)
    {
        inner.Write(data, len);
        return *this;
    }
    void Finalize(unsigned char hash[OUTPUT_SIZE])
    {
        unsigned char temp[64];
        inner.Finalize(temp);
        outer.Write(temp, 64).Finalize(hash);
    }
};