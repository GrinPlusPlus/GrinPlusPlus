#pragma once

#include <Models/TxModels.h>

#include <Core/Models/FullBlock.h>
#include <Wallet/KeyChainPath.h>
#include <memory>
#include <optional>

struct MinedBlock
{
    //std::shared_ptr<MinedBlock> pParent;

    FullBlock block;
    uint64_t coinbaseAmount;
    std::optional<KeyChainPath> coinbasePath;

    std::vector<Test::Tx> txs;
};