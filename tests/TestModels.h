#pragma once

#include <Core/Models/FullBlock.h>
#include <Wallet/KeyChainPath.h>
#include <optional>

struct MinedBlock
{
    FullBlock block;
    uint64_t coinbaseAmount;
    std::optional<KeyChainPath> coinbasePath;
};