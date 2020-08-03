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

void WalletSqlite::OnInitWrite()
{
	m_pTransaction = std::make_unique<SqliteTransaction>(SqliteTransaction(*m_pDatabase));
	m_pTransaction->Begin();
	SetDirty(true);
}

void WalletSqlite::OnEndWrite()
{
	m_pTransaction.reset();
}

KeyChainPath WalletSqlite::GetNextChildPath(const KeyChainPath& parentPath)
{
	sqlite3_stmt* stmt = nullptr;
	const std::string query = "SELECT next_child_index FROM accounts WHERE parent_path='" + parentPath.Format() + "'";
	if (sqlite3_prepare_v2(m_pDatabase, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(m_pDatabase));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		WALLET_ERROR_F("Account not found for user: {}", m_username);
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Account not found.");
	}

	const uint32_t nextChildIndex = (uint32_t)sqlite3_column_int(stmt, 0);
	KeyChainPath nextChildPath = parentPath.GetChild(nextChildIndex);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(m_pDatabase));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	char* error = nullptr;
	const std::string update = "UPDATE accounts SET next_child_index=" + std::to_string(nextChildPath.GetKeyIndices().back() + 1) + " WHERE parent_path='" + parentPath.Format() + "';";
	if (sqlite3_exec(m_pDatabase, update.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		WALLET_ERROR_F("Failed to update account for user: {}. Error: {}", m_username, error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to update account");
	}

	return nextChildPath;
}

std::unique_ptr<Slate> WalletSqlite::LoadSlate(const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateStage& stage) const
{
	return SlateTable::LoadSlate(*m_pDatabase, masterSeed, slateId, stage);
}

void WalletSqlite::SaveSlate(const SecureVector& masterSeed, const Slate& slate)
{
	SlateTable::SaveSlate(*m_pDatabase, masterSeed, slate);
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