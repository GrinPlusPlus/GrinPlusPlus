#pragma once

#include <libsqlite3/sqlite3.h>
#include <Common/Secure.h>
#include <uuid.h>
#include <Wallet/Models/Slate/SlateStage.h>
#include <Wallet/WalletDB/Models/SlateContextEntity.h>

class SlateContextTable
{
public:
	static void CreateTable(sqlite3& database);
	static void UpdateSchema(sqlite3& database, const int previousVersion);

	static void SaveSlateContext(
		sqlite3& database,
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const SlateContextEntity& slateContext
	);
	
	static std::unique_ptr<SlateContextEntity> LoadSlateContext(
		sqlite3& database,
		const SecureVector& masterSeed,
		const uuids::uuid& slateId
	);

private:
	static std::vector<unsigned char> Encrypt(
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const CBigInteger<16> iv,
		const SlateContextEntity& slateContext
	);

	static SlateContextEntity Decrypt(
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const CBigInteger<16> iv,
		const std::vector<unsigned char>& encrypted
	);

	static Hash DeriveAESKey(const SecureVector& masterSeed, const uuids::uuid& slateId);
};