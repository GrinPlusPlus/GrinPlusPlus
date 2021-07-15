#include <iostream>

#include <BlockChain/BlockChain.h>
#include <Consensus.h>
#include <Common/Util/FileUtil.h>
#include <Core/Global.h>
#include <Core/Context.h>
#include <Database/Database.h>
#include <PMMR/TxHashSetManager.h>
#include <PMMR/HeaderMMR.h>
#include <TxPool/TransactionPool.h>

#include "../../src/PMMR/TxHashSetImpl.h"
#include "../../src/PMMR/Zip/TxHashSetZip.h"
#include "../../src/PMMR/KernelMMR.h"
#include "../../src/PMMR/OutputPMMR.h"
#include "../../src/PMMR/RangeProofPMMR.h"

int main(int argc, char* argv[])
{
    if (argc < 3)
    {
        std::cout << "Usage: txhashset_tool </path/to/txhashset.zip> <header_hash>" << std::endl;
        return -1;
    }

    auto pContext = Context::Create(Environment::MAINNET, Config::Default(Environment::MAINNET));
    Global::Init(pContext);
    
    std::cout << "Initializing Node" << std::endl;
    auto pDatabase = DatabaseAPI::OpenDatabase(pContext->GetConfig());
    auto pTxHashSetManager = std::make_shared<TxHashSetManager>(pContext->GetConfig());
    auto pLockedTxHashSetManager = std::make_shared<Locked<TxHashSetManager>>(pTxHashSetManager);
    auto pTransactionPool = TxPoolAPI::CreateTransactionPool(pContext->GetConfig());
    auto pHeaderMMR = HeaderMMRAPI::OpenHeaderMMR(pContext->GetConfig());
    auto pBlockChain = BlockChainAPI::OpenBlockChain(
        pContext->GetConfig(),
        pDatabase->GetBlockDB(),
        pLockedTxHashSetManager,
        pTransactionPool,
        pHeaderMMR
    );

	const TxHashSetZip zip(pContext->GetConfig());

    std::string hash_hex(argv[2]);
    auto pHeader = pBlockChain->GetBlockHeaderByHash(Hash::FromHex(hash_hex));
    if (!pHeader) {
        std::cout << "Header not found" << std::endl;
        return -2;
    }

    if (!zip.Extract(argv[1], *pHeader)) {
        std::cout << "Failed to extract zip" << std::endl;
        return -3;
    }

    std::string txhashset_path = pContext->GetConfig().GetTxHashSetPath();

    // Rewind Kernel MMR
    std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(txhashset_path);
    pKernelMMR->Rewind(pHeader->GetNumKernels());
    pKernelMMR->Commit();

    // Rewind Output MMR
    std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(txhashset_path);
    pOutputPMMR->Rewind(pHeader->GetNumOutputs(), {});
    pOutputPMMR->Commit();

    // Rewind RangeProof MMR
    std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(txhashset_path);
    pRangeProofPMMR->Rewind(pHeader->GetNumOutputs(), {});
    pRangeProofPMMR->Commit();

    TxHashSet txhashset(pKernelMMR, pOutputPMMR, pRangeProofPMMR, pHeader);

    for (Index mmr_idx = Index::At(0); mmr_idx < 10; mmr_idx++) {
        std::cout << mmr_idx.Get() << ": " << pOutputPMMR->IsCompacted(mmr_idx) << std::endl;
    }

    const uint64_t size = pOutputPMMR->GetSize();
    for (Index mmr_idx = Index::At(0); mmr_idx < size; mmr_idx++) {
        if (!mmr_idx.IsLeaf()) {
            const std::unique_ptr<Hash> pParentHash = pOutputPMMR->GetHashAt(mmr_idx);
            if (pParentHash != nullptr) {
                const std::unique_ptr<Hash> pLeftHash = pOutputPMMR->GetHashAt(mmr_idx.GetLeftChild());
                const std::unique_ptr<Hash> pRightHash = pOutputPMMR->GetHashAt(mmr_idx.GetRightChild());
                if (pLeftHash != nullptr && pRightHash != nullptr) {
                    const Hash expectedHash = MMRHashUtil::HashParentWithIndex(*pLeftHash, *pRightHash, mmr_idx.GetPosition());
                    if (*pParentHash != expectedHash) {
                        std::cout << "Invalid parent hash at " << mmr_idx.Get() << std::endl;
                        return -4;
                    }
                }
            }
        }
    }

    return 0;
}
