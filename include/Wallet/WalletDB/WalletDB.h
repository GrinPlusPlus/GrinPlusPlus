#pragma once

#include <string>
#include <vector>

#include <uuid.h>
#include <Core/Traits/Batchable.h>
#include <Core/Traits/Lockable.h>
#include <Wallet/EncryptedSeed.h>
#include <Wallet/KeyChainPath.h>
#include <Wallet/SlateContext.h>
#include <Wallet/OutputData.h>
#include <Wallet/WalletTx.h>

class IWalletDB : public Traits::IBatchable
{
public:
	virtual ~IWalletDB() = default;

	virtual KeyChainPath GetNextChildPath(const KeyChainPath& parentPath) = 0;

	virtual std::unique_ptr<SlateContext> LoadSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId) const = 0;
	virtual void SaveSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext) = 0;

	virtual void AddOutputs(const SecureVector& masterSeed, const std::vector<OutputData>& outputs) = 0;
	virtual std::vector<OutputData> GetOutputs(const SecureVector& masterSeed) const = 0;

	virtual void AddTransaction(const SecureVector& masterSeed, const WalletTx& walletTx) = 0;
	virtual std::vector<WalletTx> GetTransactions(const SecureVector& masterSeed) const = 0;
	virtual std::unique_ptr<WalletTx> GetTransactionById(const SecureVector& masterSeed, const uint32_t walletTxId) const = 0;

	virtual uint32_t GetNextTransactionId() = 0;
	virtual uint64_t GetRefreshBlockHeight() const = 0;
	virtual void UpdateRefreshBlockHeight(const uint64_t refreshBlockHeight) = 0;
	virtual uint64_t GetRestoreLeafIndex() const = 0;
	virtual void UpdateRestoreLeafIndex(const uint64_t lastLeafIndex) = 0;
};

typedef std::shared_ptr<IWalletDB> IWalletDBPtr;
