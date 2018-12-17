#pragma once

#include <BigInteger.h>

// TODO: Find better location for this.
typedef CBigInteger<32> Hash;

static const Hash ZERO_HASH = { Hash::ValueOf(0) };
static const int HASH_SIZE = 32;