#pragma once

#include <Common/Secure.h>
#include <uuid.h>
#include <Wallet/Models/Slate/SlateStage.h>
#include <Wallet/WalletDB/Models/SlateContextEntity.h>

#include "../SqliteDB.h"

class SlateContextTable
{
public:
	static void CreateTable(SqliteDB& database);
	static void UpdateSchema(SqliteDB& database, const int previousVersion);

	static void SaveSlateContext(
		SqliteDB& database,
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const SlateContextEntity& slateContext
	);
	
	static std::unique_ptr<SlateContextEntity> LoadSlateContext(
		SqliteDB& database,
		const SecureVector& masterSeed,
		const uuids::uuid& slateId
	);

private:
	static std::vector<unsigned char> Encrypt(
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const CBigInteger<16>& iv,
		const SlateContextEntity& slateContext
	);

	static SlateContextEntity Decrypt(
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const CBigInteger<16>& iv,
		const std::vector<unsigned char>& encrypted
	);

	static Hash DeriveAESKey(const SecureVector& masterSeed, const uuids::uuid& slateId);
};