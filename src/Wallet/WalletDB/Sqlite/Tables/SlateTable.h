#pragma once

#include <libsqlite3/sqlite3.h>
#include <Common/Secure.h>
#include <uuid.h>
#include <Wallet/Models/Slate/Slate.h>

class SlateTable
{
public:
	static void CreateTable(sqlite3& database);
	static void UpdateSchema(sqlite3& database, const int previousVersion);

	static std::unique_ptr<Slate> LoadSlate(
		sqlite3& database,
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const SlateStage& stage
	);

	static void SaveSlate(
		sqlite3& database,
		const SecureVector& masterSeed,
		const Slate& slate
	);

private:
	static std::vector<uint8_t> Encrypt(
		const SecureVector& masterSeed,
		const CBigInteger<16>& iv,
		const Slate& slate
	);
	static Slate Decrypt(
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const CBigInteger<16>& iv,
		const std::vector<uint8_t>& encrypted
	);

	static Hash DeriveAESKey(const SecureVector& masterSeed, const uuids::uuid& slateId);
};