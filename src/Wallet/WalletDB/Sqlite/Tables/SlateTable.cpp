#include "SlateTable.h"

#include <Common/Util/StringUtil.h>
#include <Crypto/RandomNumberGenerator.h>
#include <Infrastructure/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>

void SlateTable::CreateTable(sqlite3& database)
{
	std::string tableCreation = "create table slate(slate_id TEXT NOT NULL, stage TEXT NOT NULL, iv BLOB NOT NULL, slate BLOB NOT NULL);";
	// TODO: Add indices

	char* error = nullptr;
	if (sqlite3_exec(&database, tableCreation.c_str(), NULL, NULL, &error) != SQLITE_OK)
	{
		WALLET_ERROR_F("Create slate table failed with error: {}", error);
		sqlite3_free(error);
		throw WALLET_STORE_EXCEPTION("Failed to create slate table.");
	}
}

void SlateTable::UpdateSchema(sqlite3& database, const int previousVersion)
{
	if (previousVersion < 2)
	{
		CreateTable(database);
	}
}

void SlateTable::SaveSlate(
	sqlite3& database,
	const SecureVector& masterSeed,
	const Slate& slate)
{
	// TODO: Update if it already exists?
	sqlite3_stmt* stmt = nullptr;
	std::string insert = "insert into slate values(?, ?, ?, ?)";
	if (sqlite3_prepare_v2(&database, insert.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(&database));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	const std::string slateIdStr = uuids::to_string(slate.slateId);
	sqlite3_bind_text(stmt, 1, slateIdStr.c_str(), (int)slateIdStr.size(), NULL);

	const std::string stage = slate.stage.ToString();
	sqlite3_bind_text(stmt, 2, stage.c_str(), (int)stage.size(), NULL);

	CBigInteger<16> iv(RandomNumberGenerator::GenerateRandomBytes(16).data());
	sqlite3_bind_blob(stmt, 3, (const void*)iv.data(), (int)iv.size(), NULL);

	const std::vector<unsigned char> encrypted = Encrypt(masterSeed, iv, slate);
	sqlite3_bind_blob(stmt, 4, (const void*)encrypted.data(), (int)encrypted.size(), NULL);

	sqlite3_step(stmt);

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}
}

std::unique_ptr<Slate> SlateTable::LoadSlate(
	sqlite3& database,
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const SlateStage& stage)
{
	std::unique_ptr<Slate> pSlate = nullptr;

	sqlite3_stmt* stmt = nullptr;
	const std::string query = "SELECT slate FROM slate WHERE slate_id='" + uuids::to_string(slateId) + "' and stage='" + stage.ToString() + "'";
	if (sqlite3_prepare_v2(&database, query.c_str(), -1, &stmt, NULL) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error while compiling sql: {}", sqlite3_errmsg(&database));
		sqlite3_finalize(stmt);
		throw WALLET_STORE_EXCEPTION("Error compiling statement.");
	}

	if (sqlite3_step(stmt) == SQLITE_ROW)
	{
		WALLET_DEBUG_F("Slate found for id {}", uuids::to_string(slateId));

		const int ivBytes = sqlite3_column_bytes(stmt, 0);
		if (ivBytes != 16)
		{
			WALLET_ERROR_F("Slate corrupted: {}", sqlite3_errmsg(&database));
			sqlite3_finalize(stmt);
			throw WALLET_STORE_EXCEPTION("Slate corrupted.");
		}

		CBigInteger<16> iv((const unsigned char*)sqlite3_column_blob(stmt, 0));

		const int encryptedBytes = sqlite3_column_bytes(stmt, 1);

		try
		{
			const unsigned char* pBytes = (const unsigned char*)sqlite3_column_blob(stmt, 1);
			std::vector<unsigned char> encrypted(pBytes, pBytes + encryptedBytes);

			pSlate = std::make_unique<Slate>(Decrypt(masterSeed, slateId, iv, encrypted));
		}
		catch (std::exception& e)
		{
			WALLET_ERROR_F("Failed to load slate ({}): {}", uuids::to_string(slateId), e);
			sqlite3_finalize(stmt);
			throw WALLET_STORE_EXCEPTION("Failed to load slate");
		}
	}
	else
	{
		WALLET_INFO_F("Slate not found for id {}", uuids::to_string(slateId));
	}

	if (sqlite3_finalize(stmt) != SQLITE_OK)
	{
		WALLET_ERROR_F("Error finalizing statement: {}", sqlite3_errmsg(&database));
		throw WALLET_STORE_EXCEPTION("Error finalizing statement.");
	}

	return pSlate;
}

// Binary serializes a slate, encrypts it with AES256 using a random 16-byte IV, and returns it as: `IV | encrypted_slate`
std::vector<uint8_t> SlateTable::Encrypt(
	const SecureVector& masterSeed,
	const CBigInteger<16>& iv,
	const Slate& slate)
{
	// Encrypt slate
	Serializer serializer;
	slate.Serialize(serializer);
	std::vector<uint8_t> encryptedSlate = Crypto::AES256_Encrypt(serializer.GetSecureBytes(), DeriveAESKey(masterSeed, slate.slateId), iv);

	return encryptedSlate;
}

Slate SlateTable::Decrypt(
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const CBigInteger<16>& iv,
	const std::vector<uint8_t>& encrypted)
{
	SecretKey aesKey = DeriveAESKey(masterSeed, slateId);
	SecureVector decrypted = Crypto::AES256_Decrypt(encrypted, aesKey, iv);

	ByteBuffer byteBuffer(std::vector<unsigned char>{ decrypted.begin(), decrypted.end() });
	return Slate::Deserialize(byteBuffer);
}

Hash SlateTable::DeriveAESKey(const SecureVector& masterSeed, const uuids::uuid& slateId)
{
	Serializer serializer;
	serializer.AppendByteVector((const std::vector<unsigned char>&)masterSeed);
	serializer.AppendVarStr(uuids::to_string(slateId));
	serializer.AppendVarStr("slate");

	return Crypto::Blake2b(serializer.GetBytes());
}