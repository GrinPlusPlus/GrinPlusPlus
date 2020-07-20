// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

/**
 * Utilities for converting data from/to strings.
 */
#ifndef BITCOIN_UTIL_STRENCODINGS_H
#define BITCOIN_UTIL_STRENCODINGS_H

/**
 * Tests if the given character is a whitespace character. The whitespace characters
 * are: space, form-feed ('\f'), newline ('\n'), carriage return ('\r'), horizontal
 * tab ('\t'), and vertical tab ('\v').
 *
 * This function is locale independent. Under the C locale this function gives the
 * same result as std::isspace.
 *
 * @param[in] c     character to test
 * @return          true if the argument is a whitespace character; otherwise false
 */
constexpr inline bool IsSpace(char c) noexcept {
    return c == ' ' || c == '\f' || c == '\n' || c == '\r' || c == '\t' || c == '\v';
}

#endif // BITCOIN_UTIL_STRENCODINGS_H