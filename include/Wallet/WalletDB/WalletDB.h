#pragma once

#include <string>
#include <vector>

#include <uuid.h>
#include <Core/Traits/Batchable.h>
#include <Core/Traits/Lockable.h>
#include <Wallet/WalletDB/Models/EncryptedSeed.h>
#include <Wallet/KeyChainPath.h>
#include <Wallet/WalletDB/Models/SlateContextEntity.h>
#include <Wallet/Models/Slate/Slate.h>
#include <Wallet/WalletDB/Models/OutputDataEntity.h>
#include <Wallet/WalletTx.h>

class IWalletDB : public Traits::IBatchable
{
public:
	virtual ~IWalletDB() = default;

	virtual KeyChainPath GetNextChildPath(const KeyChainPath& parentPath) = 0;

	virtual std::unique_ptr<Slate> LoadSlate(
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const SlateStage& stage
	) const = 0;
	virtual std::pair<std::unique_ptr<Slate>, std::string> LoadLatestSlate(
		const SecureVector& masterSeed,
		const uuids::uuid& slateId
	) const = 0;
	virtual std::string LoadArmoredSlatepack(const uuids::uuid& slateId) const = 0;

	virtual void SaveSlate(
		const SecureVector& masterSeed,
		const Slate& slate,
		const std::string& slatepack_message
	) = 0;

	virtual std::unique_ptr<SlateContextEntity> LoadSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId) const = 0;
	virtual void SaveSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContextEntity& slateContext) = 0;

	virtual void AddOutputs(const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs) = 0;
	virtual std::vector<OutputDataEntity> GetOutputs(const SecureVector& masterSeed) const = 0;

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
