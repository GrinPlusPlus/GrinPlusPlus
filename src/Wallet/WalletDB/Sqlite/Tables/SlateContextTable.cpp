#include "SlateContextTable.h"

#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>

void SlateContextTable::CreateTable(sqlite3& database)
{
	std::string tableCreation = "create table slate_contexts(slate_id TEXT PRIMARY KEY, iv BLOB NOT NULL, enc_context BLOB NOT NULL);";
	// TODO: Add indices

	char* error = nullptr;
	if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		WALLET_ERROR_F("Create slate_contexts table failed with error: {}", error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to create slate_contexts table.");
	}
}

void SlateContextTable::UpdateSchema(sqlite3& database, const int previousVersion)
{
	if (previousVersion < 2)
	{
		std::string update = "DROP TABLE slate_contexts;";
		char* error = nullptr;
		if (sqlite3_exec(&database, update.c_str(), NULL, NULL, &error) != SQLITE_OK)
		{
			WALLET_ERROR_F("Failed to drop slate_contexts table. Error: {}", error);
			sqlite3_free(error);
			throw WALLET_STORE_EXCEPTION("Failed to drop slate_contexts table.");
		}

		CreateTable(database);
	}
}

void SlateContextTable::SaveSlateContext(
	sqlite3& database,
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const SlateContextEntity& slateContext)
{
	sqlite3_stmt* stmt = nullptr;
	std::string insert = "insert into slate_contexts values(?, ?, ?)";
	if (sqlite3_prepare_v2(&database, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(&database));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	const std::string slateIdStr = uuids::to_string(slateId);
	sqlite3_bind_text(stmt, 1, slateIdStr.c_str(), (int)slateIdStr.size(), NULL);

	CBigInteger<16> iv(RandomNumberGenerator::GenerateRandomBytes(16).data());
	sqlite3_bind_blob(stmt, 2, (const void*)iv.data(), 16, NULL);

	std::vector<uint8_t> encrypted = Encrypt(masterSeed, slateId, iv, slateContext);
	sqlite3_bind_blob(stmt, 3, (const void*)encrypted.data(), (int)encrypted.size(), NULL);

	sqlite3_step(stmt);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}
}

std::unique_ptr<SlateContextEntity> SlateContextTable::LoadSlateContext(
	sqlite3& database,
	const SecureVector& masterSeed,
	const uuids::uuid& slateId)
{
	std::unique_ptr<SlateContextEntity> pSlateContext = nullptr;

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "SELECT iv, enc_context FROM slate_contexts WHERE slate_id='" + uuids::to_string(slateId) + "'";
	if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(&database));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		WALLET_DEBUG_F("Slate context found for id {}", uuids::to_string(slateId));

		const int ivBytes = sqlite3_column_bytes(stmt, 0);
		if (ivBytes != 16)
		{
			WALLET_ERROR_F("Slate context corrupted: {}", sqlite3_errmsg(&database));
			sqlite3_finalize(stmt);
			throw WALLET_STORE_EXCEPTION("Slate context corrupted.");
		}

		CBigInteger<16> iv((const unsigned char*)sqlite3_column_blob(stmt, 0));

		const int numBytes = sqlite3_column_bytes(stmt, 1);
		const unsigned char* pEncryptedBytes = (const unsigned char*)sqlite3_column_blob(stmt, 1);
		std::vector<unsigned char> encrypted(pEncryptedBytes, pEncryptedBytes + numBytes);

		pSlateContext = std::make_unique<SlateContextEntity>(Decrypt(masterSeed, slateId, iv, encrypted)); // TODO: Pass in keypath and pubkey?
	}
	else
	{
		WALLET_INFO_F("Slate context not found for id {}", uuids::to_string(slateId));
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return pSlateContext;
}

std::vector<unsigned char> SlateContextTable::Encrypt(
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const CBigInteger<16>& iv,
	const SlateContextEntity& slateContext)
{
	Serializer serializer;
	slateContext.Serialize(serializer);
	const SecretKey aesKey = DeriveAESKey(masterSeed, slateId);
	std::vector<uint8_t> encrypted = Crypto::AES256_Encrypt(serializer.GetSecureBytes(), aesKey, iv);

	return encrypted;
}

SlateContextEntity SlateContextTable::Decrypt(
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const CBigInteger<16>& iv,
	const std::vector<unsigned char>& encrypted)
{
	const SecretKey aesKey = DeriveAESKey(masterSeed, slateId);
	SecureVector decrypted = Crypto::AES256_Decrypt(encrypted, aesKey, iv);

	ByteBuffer byteBuffer(std::vector<unsigned char>{ decrypted.begin(), decrypted.end() });
	return SlateContextEntity::Deserialize(byteBuffer);
}

Hash SlateContextTable::DeriveAESKey(const SecureVector& masterSeed, const uuids::uuid& slateId)
{
	Serializer serializer;
	serializer.AppendByteVector((const std::vector<unsigned char>&)masterSeed);
	serializer.AppendVarStr(uuids::to_string(slateId));
	serializer.AppendVarStr("context");

	return Crypto::Blake2b(serializer.GetBytes());
}