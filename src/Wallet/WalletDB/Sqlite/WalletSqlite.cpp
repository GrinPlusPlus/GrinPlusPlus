#include "WalletSqlite.h"
#include "../WalletEncryptionUtil.h"
#include "SqliteTransaction.h"
#include "Tables/VersionTable.h"
#include "Tables/OutputsTable.h"
#include "Tables/TransactionsTable.h"
#include "Tables/MetadataTable.h"
#include "Tables/SlateTable.h"
#include "Tables/SlateContextTable.h"

#include <Wallet/WalletDB/WalletStoreException.h>
#include <Wallet/Models/Slate/SlateStage.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>

static const uint8_t ENCRYPTION_FORMAT = 0;
static const int LATEST_SCHEMA_VERSION = 1;

void WalletSqlite::Commit()
{
	m_pTransaction->Commit();
	SetDirty(false);
}

void WalletSqlite::Rollback() noexcept
{
	m_pTransaction->Rollback();
	SetDirty(false);
}

void WalletSqlite::OnInitWrite(const bool /*batch*/)
{
	m_pTransaction = std::make_unique<SqliteTransaction>(m_pDatabase);
	m_pTransaction->Begin();
	SetDirty(true);
}

void WalletSqlite::OnEndWrite()
{
	m_pTransaction.reset();
}

KeyChainPath WalletSqlite::GetNextChildPath(const KeyChainPath& parentPath)
{
	std::string get_next_child_query = StringUtil::Format("SELECT next_child_index FROM accounts WHERE parent_path='{}'", parentPath.Format());
	auto pStatement = m_pDatabase->Query(get_next_child_query);

	if (!pStatement->Step()) {
		WALLET_ERROR_F("Account not found for user: {}", m_username);
		throw WALLET_STORE_EXCEPTION("Account not found.");
	}

	const uint32_t nextChildIndex = (uint32_t)pStatement->GetColumnInt(0);
	KeyChainPath nextChildPath = parentPath.GetChild(nextChildIndex);
	pStatement->Finalize();

	std::string update_next_child_cmd = StringUtil::Format(
		"UPDATE accounts SET next_child_index={} WHERE parent_path='{}';",
		nextChildPath.GetKeyIndices().back() + 1,
		parentPath.Format()
	);
	m_pDatabase->Execute(update_next_child_cmd);

	return nextChildPath;
}

std::unique_ptr<Slate> WalletSqlite::LoadSlate(const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateStage& stage) const
{
	return SlateTable::LoadSlate(*m_pDatabase, masterSeed, slateId, stage);
}

std::pair<std::unique_ptr<Slate>, std::string> WalletSqlite::LoadLatestSlate(const SecureVector& masterSeed, const uuids::uuid& slateId) const
{
	return SlateTable::LoadLatestSlate(*m_pDatabase, masterSeed, slateId);
}

std::string WalletSqlite::LoadArmoredSlatepack(const uuids::uuid& slateId) const
{
	return SlateTable::LoadArmoredSlatepack(*m_pDatabase, slateId);
}

void WalletSqlite::SaveSlate(const SecureVector& masterSeed, const Slate& slate, const std::string& slatepack_message)
{
	SlateTable::SaveSlate(*m_pDatabase, masterSeed, slate, slatepack_message);
}

std::unique_ptr<SlateContextEntity> WalletSqlite::LoadSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId) const
{
	return SlateContextTable::LoadSlateContext(*m_pDatabase, masterSeed, slateId);
}

void WalletSqlite::SaveSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContextEntity& slateContext)
{
	SlateContextTable::SaveSlateContext(*m_pDatabase, masterSeed, slateId, slateContext);
}

void WalletSqlite::AddOutputs(const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs)
{
	OutputsTable::AddOutputs(*m_pDatabase, masterSeed, outputs);
}

std::vector<OutputDataEntity> WalletSqlite::GetOutputs(const SecureVector& masterSeed) const
{
	return OutputsTable::GetOutputs(*m_pDatabase, masterSeed);
}

void WalletSqlite::AddTransaction(const SecureVector& masterSeed, const WalletTx& walletTx)
{
	TransactionsTable::AddTransactions(*m_pDatabase, masterSeed, std::vector<WalletTx>({ walletTx }));
}

std::vector<WalletTx> WalletSqlite::GetTransactions(const SecureVector& masterSeed) const
{
	return TransactionsTable::GetTransactions(*m_pDatabase, masterSeed);
}

std::unique_ptr<WalletTx> WalletSqlite::GetTransactionById(const SecureVector& masterSeed, const uint32_t walletTxId) const
{
	return TransactionsTable::GetTransactionById(*m_pDatabase, masterSeed, walletTxId);
}

uint32_t WalletSqlite::GetNextTransactionId()
{
	UserMetadata metadata = GetMetadata();

	const uint32_t nextTxId = metadata.GetNextTxId();
	const UserMetadata updatedMetadata(nextTxId + 1, metadata.GetRefreshBlockHeight(), metadata.GetRestoreLeafIndex());
	SaveMetadata(updatedMetadata);

	return nextTxId;
}

uint64_t WalletSqlite::GetRefreshBlockHeight() const
{
	UserMetadata metadata = GetMetadata();

	return metadata.GetRefreshBlockHeight();
}

void WalletSqlite::UpdateRefreshBlockHeight(const uint64_t refreshBlockHeight)
{
	UserMetadata metadata = GetMetadata();

	SaveMetadata(UserMetadata(metadata.GetNextTxId(), refreshBlockHeight, metadata.GetRestoreLeafIndex()));
}

uint64_t WalletSqlite::GetRestoreLeafIndex() const
{
	UserMetadata metadata = GetMetadata();

	return metadata.GetRestoreLeafIndex();
}

void WalletSqlite::UpdateRestoreLeafIndex(const uint64_t lastLeafIndex)
{
	UserMetadata metadata = GetMetadata();

	SaveMetadata(UserMetadata(metadata.GetNextTxId(), metadata.GetRefreshBlockHeight(), lastLeafIndex));
}

UserMetadata WalletSqlite::GetMetadata() const
{
	return MetadataTable::GetMetadata(*m_pDatabase);
}

void WalletSqlite::SaveMetadata(const UserMetadata& userMetadata)
{
	MetadataTable::SaveMetadata(*m_pDatabase, userMetadata);
}