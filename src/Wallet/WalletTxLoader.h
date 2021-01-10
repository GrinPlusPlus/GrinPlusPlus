#pragma once

#include <API/Wallet/Owner/Models/ListTxsCriteria.h>
#include <Wallet/Models/DTOs/WalletTxDTO.h>
#include <Wallet/WalletTx.h>
#include <Common/Secure.h>

// Forward Declarations
class IWalletDB;

class WalletTxLoader
{
public:
    //
    // Loads all transactions, and filters them using the given criteria.
    //
    std::vector<WalletTxDTO> LoadTransactions(
        const std::shared_ptr<const IWalletDB>& pWalletDB,
        const SecureVector& masterSeed,
        const ListTxsCriteria& criteria
    ) const;

private:
    WalletTxDTO ToDTO(
        const std::shared_ptr<const IWalletDB>& pWalletDB,
        const SecureVector& masterSeed,
        const std::vector<OutputDataEntity>& outputs,
        const WalletTx& walletTx
    ) const;

    std::vector<WalletTx> LoadWalletTxs(
        const std::shared_ptr<const IWalletDB>& pWalletDB,
        const SecureVector& masterSeed,
        const ListTxsCriteria& criteria
    ) const;

    bool MeetsCriteria(
        const WalletTx& walletTx,
        const ListTxsCriteria& criteria
    ) const;
};