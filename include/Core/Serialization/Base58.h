// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Copyright (c) 2020 David Burkett
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Why base-58 instead of standard base-64 encoding?
 * - Don't want 0OIl characters that look the same in some fonts and
 *      could be used to create visually identical looking data.
 * - A string with non-alphanumeric characters is not as easily accepted as input.
 * - E-mail usually won't line-break if there's no punctuation to break at.
 * - Double-clicking selects the whole string as one word if it's all alphanumeric.
 */
#ifndef BITCOIN_BASE58_H
#define BITCOIN_BASE58_H

#include <Crypto/Models/Hash.h>

#include <string>
#include <vector>

class Base58
{
public:
    /**
     * Encode a byte sequence as a base58-encoded string.
     * pbegin and pend cannot be nullptr, unless both are.
     */
    static std::string Encode(const unsigned char* pbegin, const unsigned char* pend, const bool include_version);

    /**
     * Encode a byte vector as a base58-encoded string
     */
    static std::string Encode(const std::vector<unsigned char>& vch);

    /**
     * Decode a base58-encoded string (psz) into a byte vector (vchRet).
     * return true if decoding is successful.
     * psz cannot be nullptr.
     */
    static std::vector<uint8_t> Decode(const char* psz, const bool include_version);

    /**
     * Decode a base58-encoded string (str) into a byte vector (vchRet).
     * return true if decoding is successful.
     */
    static std::vector<uint8_t> Decode(const std::string& str);

    /**
     * Encode a byte vector into a base58-encoded string, including checksum
     */
    static std::string EncodeCheck(const std::vector<unsigned char>& vchIn);

    /**
     * Decode a base58-encoded string (psz) that includes a checksum into a byte
     * vector (vchRet), return true if decoding is successful
     */
    static std::vector<uint8_t> DecodeCheck(const char* psz);

    /**
     * Decode a base58-encoded string (str) that includes a checksum into a byte
     * vector (vchRet), return true if decoding is successful
     */
    static std::vector<uint8_t> DecodeCheck(const std::string& str);

    static std::string SimpleEncodeCheck(const std::vector<unsigned char>& vchIn);

    static std::vector<uint8_t> SimpleDecodeCheck(const std::string& str);

private:
    static Hash SHA256d(
        const std::vector<uint8_t>::const_iterator& pbegin,
        const std::vector<uint8_t>::const_iterator& pend
    );
};

#endif // BITCOIN_BASE58_H
