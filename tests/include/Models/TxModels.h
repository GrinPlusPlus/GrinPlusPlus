#pragma once

#include <Core/Models/Transaction.h>
#include <Wallet/KeyChainPath.h>

namespace Test
{
    struct Input
    {
        TransactionInput input;
        KeyChainPath path;
        uint64_t amount;
    };

    struct Output
    {
        KeyChainPath path;
        uint64_t amount;
    };

    struct Tx
    {
        TransactionPtr pTransaction;
        std::vector<Input> inputs;
        std::vector<Output> outputs;
    };
}