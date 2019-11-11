#include "WalletSqlite.h"
#include "../WalletEncryptionUtil.h"
#include "SqliteTransaction.h"
#include "Tables/VersionTable.h"
#include "Tables/OutputsTable.h"
#include "Tables/TransactionsTable.h"
#include "Tables/MetadataTable.h"

#include <Wallet/WalletDB/WalletStoreException.h>
#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

static const uint8_t ENCRYPTION_FORMAT = 0;
static const int LATEST_SCHEMA_VERSION = 1;

void WalletSqlite::Commit()
{
	m_pTransaction->Commit();
	SetDirty(false);
}

void WalletSqlite::Rollback()
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
		WALLET_ERROR_F("Error while compiling sql: %s", sqlite3_errmsg(m_pDatabase));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) != SQLITE_ROW)
	{
		WALLET_ERROR_F("Account not found for user: %s", m_username);
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Account not found.");
	}

	const uint32_t nextChildIndex = (uint32_t)sqlite3_column_int(stmt, 0);
	KeyChainPath nextChildPath = parentPath.GetChild(nextChildIndex);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: %s", sqlite3_errmsg(m_pDatabase));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	char* error = nullptr;
	const std::string update = "UPDATE accounts SET next_child_index=" + std::to_string(nextChildPath.GetKeyIndices().back() + 1) + " WHERE parent_path='" + parentPath.Format() + "';";
	if (sqlite3_exec(m_pDatabase, update.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		WALLET_ERROR_F("Failed to update account for user: %s. Error: %s", m_username, error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to update account");
	}

	return nextChildPath;
}

std::unique_ptr<SlateContext> WalletSqlite::LoadSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId) const
{
	std::unique_ptr<SlateContext> pSlateContext = nullptr;

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "SELECT enc_blind, enc_nonce FROM slate_contexts WHERE slate_id='" + uuids::to_string(slateId) + "'";
	if (sqlite3_prepare_v2(m_pDatabase, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: %s", sqlite3_errmsg(m_pDatabase));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		WALLET_DEBUG_F("Slate context found for user %s", m_username);

		const int blindBytes = sqlite3_column_bytes(stmt, 0);
		const int nonceBytes = sqlite3_column_bytes(stmt, 1);
		if (blindBytes != 32 || nonceBytes != 32)
		{
			WALLET_ERROR("Blind or nonce not 32 bytes.");
			sqlite3_finalize(stmt);
			throw WALLET_STORE_EXCEPTION("Slate context corrupt");
		}

		CBigInteger<32> encryptedBlindingFactor((const unsigned char*)sqlite3_column_blob(stmt, 0));
		SecretKey blindingFactor = encryptedBlindingFactor ^ SlateContext::DeriveXORKey(masterSeed, slateId, "blind");

		CBigInteger<32> encryptedNonce((const unsigned char*)sqlite3_column_blob(stmt, 1));
		SecretKey nonce = encryptedNonce ^ SlateContext::DeriveXORKey(masterSeed, slateId, "nonce");

		pSlateContext = std::make_unique<SlateContext>(SlateContext(std::move(blindingFactor), std::move(nonce)));
	}
	else
	{
		WALLET_INFO_F("Slate context not found for user %s", m_username);
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: %s", sqlite3_errmsg(m_pDatabase));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return pSlateContext;
}

void WalletSqlite::SaveSlateContext(const SecureVector& masterSeed, const uuids::uuid& slateId, const SlateContext& slateContext)
{
	sqlite3_stmt* stmt = nullptr;
	std::string insert = "insert into slate_contexts values(?, ?, ?)";
	if (sqlite3_prepare_v2(m_pDatabase, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: %s", sqlite3_errmsg(m_pDatabase));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	const std::string slateIdStr = uuids::to_string(slateId);
	sqlite3_bind_text(stmt, 1, slateIdStr.c_str(), (int)slateIdStr.size(), NULL);

	CBigInteger<32> encryptedBlindingFactor = slateContext.GetSecretKey().GetBytes() ^ SlateContext::DeriveXORKey(masterSeed, slateId, "blind");
	sqlite3_bind_blob(stmt, 2, (const void*)encryptedBlindingFactor.data(), 32, NULL);

	CBigInteger<32> encryptedNonce = slateContext.GetSecretNonce().GetBytes() ^ SlateContext::DeriveXORKey(masterSeed, slateId, "nonce");
	sqlite3_bind_blob(stmt, 3, (const void*)encryptedNonce.data(), 32, NULL);

	sqlite3_step(stmt);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: %s", sqlite3_errmsg(m_pDatabase));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}
}

void WalletSqlite::AddOutputs(const SecureVector& masterSeed, const std::vector<OutputData>& outputs)
{
	OutputsTable::AddOutputs(*m_pDatabase, masterSeed, outputs);
}

std::vector<OutputData> WalletSqlite::GetOutputs(const SecureVector& masterSeed) const
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