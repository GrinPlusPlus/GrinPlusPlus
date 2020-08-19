// Copyright (c) 2014-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Serialization/Base58.h>
#include <Crypto/Hasher.h>
#include <bitcoin/util/strencodings.h>

#include <assert.h>
#include <string.h>

/** All alphanumeric characters except for "0", "I", "O", and "l" */
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";
static const int8_t mapBase58[256] = {
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1, 0, 1, 2, 3, 4, 5, 6,  7, 8,-1,-1,-1,-1,-1,-1,
    -1, 9,10,11,12,13,14,15, 16,-1,17,18,19,20,21,-1,
    22,23,24,25,26,27,28,29, 30,31,32,-1,-1,-1,-1,-1,
    -1,33,34,35,36,37,38,39, 40,41,42,43,-1,44,45,46,
    47,48,49,50,51,52,53,54, 55,56,57,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1, -1,-1,-1,-1,-1,-1,-1,-1,
};

std::vector<uint8_t> Base58::Decode(const char* psz, const bool include_version)
{
    // Skip leading spaces.
    while (*psz && IsSpace(*psz)) {
        psz++;
    }

    // Skip and count leading '1's.
    int zeroes = 0;
    int length = 0;

    if (include_version) {
        while (*psz == '1') {
            zeroes++;
            psz++;
        }
    }

    // Allocate enough space in big-endian base256 representation.
    size_t size = strlen(psz) * 733 /1000 + 1; // log(58) / log(256), rounded up.
    std::vector<unsigned char> b256(size);
    // Process the characters.
    static_assert(sizeof(mapBase58)/sizeof(mapBase58[0]) == 256, "mapBase58.size() should be 256"); // guarantee not out of range

    while (*psz && !IsSpace(*psz)) {
        // Decode base58 character
        int carry = mapBase58[(uint8_t)*psz];
        // Invalid b58 character
        if (carry == -1) {
            throw std::exception();
        }

        int i = 0;
        for (std::vector<unsigned char>::reverse_iterator it = b256.rbegin(); (carry != 0 || i < length) && (it != b256.rend()); ++it, ++i) {
            carry += 58 * (*it);
            *it = carry % 256;
            carry /= 256;
        }

        assert(carry == 0);
        length = i;
        psz++;
    }

    // Skip trailing spaces.
    while (IsSpace(*psz)) {
        psz++;
    }

    if (*psz != 0) {
        throw std::exception();
    }

    // Skip leading zeroes in b256.
    std::vector<unsigned char>::iterator it = b256.begin() + (size - length);
    while (it != b256.end() && *it == 0) {
        it++;
    }

    // Copy result into output vector.
    std::vector<uint8_t> result;
    result.reserve(zeroes + (b256.end() - it));
    result.assign(zeroes, 0x00);

    while (it != b256.end()) {
        result.push_back(*(it++));
    }

    return result;
}

std::string Base58::Encode(const unsigned char* pbegin, const unsigned char* pend, const bool include_version)
{
    // Skip & count leading zeroes.
    int zeroes = 0;
    int length = 0;

    if (include_version) {
        while (pbegin != pend && *pbegin == 0) {
            pbegin++;
            zeroes++;
        }
    }

    // Allocate enough space in big-endian base58 representation.
    size_t size = (pend - pbegin) * 138 / 100 + 1; // log(256) / log(58), rounded up.
    std::vector<unsigned char> b58(size);
    // Process the bytes.
    while (pbegin != pend) {
        int carry = *pbegin;
        int i = 0;
        // Apply "b58 = b58 * 256 + ch".
        for (std::vector<unsigned char>::reverse_iterator it = b58.rbegin(); (carry != 0 || i < length) && (it != b58.rend()); it++, i++) {
            carry += 256 * (*it);
            *it = carry % 58;
            carry /= 58;
        }

        assert(carry == 0);
        length = i;
        pbegin++;
    }
    // Skip leading zeroes in base58 result.
    std::vector<unsigned char>::iterator it = b58.begin() + (size - length);
    while (it != b58.end() && *it == 0)
        it++;
    // Translate the result into a string.
    std::string str;
    str.reserve(zeroes + (b58.end() - it));
    str.assign(zeroes, '1');
    while (it != b58.end())
        str += pszBase58[*(it++)];
    return str;
}

std::string Base58::Encode(const std::vector<unsigned char>& vch)
{
    return Encode(vch.data(), vch.data() + vch.size(), true);
}

std::vector<uint8_t> Base58::Decode(const std::string& str)
{
    return Decode(str.c_str(), true);
}

std::string Base58::EncodeCheck(const std::vector<unsigned char>& vchIn)
{
    // add 4-byte hash check to the end
    std::vector<unsigned char> vch(vchIn);
    Hash hash = SHA256d(vch.cbegin(), vch.cend());
    vch.insert(vch.end(), hash.data(), hash.data() + 4);
    return Encode(vch);
}

std::vector<uint8_t> Base58::DecodeCheck(const char* psz)
{
    std::vector<uint8_t> decoded = Decode(psz, true);

    if (decoded.size() < 4) {
        throw std::exception();
    }

    // re-calculate the checksum, ensure it matches the included 4-byte checksum
    Hash hash = SHA256d(decoded.begin(), decoded.end() - 4);
    if (memcmp(hash.data(), &decoded[decoded.size() - 4], 4) != 0) {
        throw std::exception();
    }

    decoded.resize(decoded.size() - 4);
    return decoded;
}

std::vector<uint8_t> Base58::DecodeCheck(const std::string& str)
{
    return DecodeCheck(str.c_str());
}

std::string Base58::SimpleEncodeCheck(const std::vector<unsigned char>& vchIn)
{
    // add 4-byte hash check to the beginning
    Hash hash = SHA256d(vchIn.cbegin(), vchIn.cend());
    std::vector<unsigned char> vch(hash.GetData().cbegin(), hash.GetData().cbegin() + 4);

    vch.insert(vch.end(), vchIn.cbegin(), vchIn.cend());

    return Encode(vch.data(), vch.data() + vch.size(), false);
}

std::vector<uint8_t> Base58::SimpleDecodeCheck(const std::string& str)
{
    std::vector<uint8_t> decoded = Decode(str.data(), false);

    if (decoded.size() < 4) {
        throw std::exception();
    }

    // re-calculate the checksum, ensure it matches the included 4-byte checksum
    Hash hash = SHA256d(decoded.begin() + 4, decoded.end());
    if (memcmp(hash.data(), decoded.data(), 4) != 0) {
        throw std::exception();
    }

    return std::vector<uint8_t>(decoded.cbegin() + 4, decoded.cend());
}

Hash Base58::SHA256d(
    const std::vector<uint8_t>::const_iterator& pbegin,
    const std::vector<uint8_t>::const_iterator& pend)
{
    std::vector<uint8_t> bytes{ pbegin, pend };
    return Hasher::SHA256(Hasher::SHA256(bytes).GetData());
}