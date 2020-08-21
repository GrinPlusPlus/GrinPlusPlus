#pragma once

#include <Common/Secure.h>
#include <uuid.h>
#include <Wallet/Models/Slate/Slate.h>

// Forward Declarations
class SqliteDB;

class SlateTable
{
public:
	static void CreateTable(SqliteDB& database);
	static void UpdateSchema(SqliteDB& database, const int previousVersion);

	static std::unique_ptr<Slate> LoadSlate(
		SqliteDB& database,
		const SecureVector& masterSeed,
		const uuids::uuid& slateId,
		const SlateStage& stage
	);

	static void SaveSlate(
		SqliteDB& database,
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