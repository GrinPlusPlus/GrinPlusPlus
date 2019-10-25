#pragma once

enum class EBlockChainStatus
{
    SUCCESS,
    UNKNOWN_ERROR,
    STORE_ERROR,
    TRANSACTIONS_MISSING,
    ALREADY_EXISTS,
    NOT_FOUND,
    INVALID,
    ORPHANED
};