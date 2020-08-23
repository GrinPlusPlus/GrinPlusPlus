#include "SlateContextTable.h"

#include <Common/Util/StringUtil.h>
#include <Common/Logger.h>
#include <Crypto/AES256.h>
#include <Crypto/Hasher.h>
#include <Wallet/WalletDB/WalletStoreException.h>

void SlateContextTable::CreateTable(SqliteDB& database)
{
	std::string table_creation_cmd = "create table slate_contexts(slate_id TEXT PRIMARY KEY, iv BLOB NOT NULL, enc_context BLOB NOT NULL);";
	// TODO: Add indices
	database.Execute(table_creation_cmd);
}

void SlateContextTable::UpdateSchema(SqliteDB& database, const int previousVersion)
{
	if (previousVersion < 2)
	{
		std::string drop_table_cmd = "DROP TABLE slate_contexts;";
		database.Execute(drop_table_cmd);

		CreateTable(database);
	}
}

void SlateContextTable::SaveSlateContext(
	SqliteDB& database,
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const SlateContextEntity& slateContext)
{
	std::string insert_context_cmd = "insert into slate_contexts values(?, ?, ?)";

	const std::string slateIdStr = uuids::to_string(slateId);
	CBigInteger<16> iv(CSPRNG::GenerateRandomBytes(16).data());
	std::vector<uint8_t> encrypted = Encrypt(masterSeed, slateId, iv, slateContext);

	std::vector<SqliteDB::IParameter::UPtr> parameters;
	parameters.push_back(TextParameter::New(slateIdStr));
	parameters.push_back(BlobParameter::New(iv.GetData()));
	parameters.push_back(BlobParameter::New(encrypted));

	database.Update(insert_context_cmd, parameters);
}

std::unique_ptr<SlateContextEntity> SlateContextTable::LoadSlateContext(
	SqliteDB& database,
	const SecureVector& masterSeed,
	const uuids::uuid& slateId)
{
	std::string get_context_query = StringUtil::Format("SELECT iv, enc_context FROM slate_contexts WHERE slate_id='{}'", uuids::to_string(slateId));
	auto pStatement = database.Query(get_context_query);

	if (!pStatement->Step()) {
		WALLET_INFO_F("Slate context not found for id {}", uuids::to_string(slateId));
		return nullptr;
	}

	WALLET_DEBUG_F("Slate context found for id {}", uuids::to_string(slateId));

	std::vector<uint8_t> iv_bytes = pStatement->GetColumnBytes(0);
	if (iv_bytes.size() != 16) {
		WALLET_ERROR_F("Slate context corrupted: {}", database.GetError());
		throw WALLET_STORE_EXCEPTION("Slate context corrupted.");
	}

	CBigInteger<16> iv(std::move(iv_bytes));
	std::vector<uint8_t> encrypted = pStatement->GetColumnBytes(1);

	return std::make_unique<SlateContextEntity>(Decrypt(masterSeed, slateId, iv, encrypted)); // TODO: Pass in keypath and pubkey?
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
	std::vector<uint8_t> encrypted = AES256::Encrypt(serializer.GetSecureBytes(), aesKey, iv);

	return encrypted;
}

SlateContextEntity SlateContextTable::Decrypt(
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const CBigInteger<16>& iv,
	const std::vector<unsigned char>& encrypted)
{
	const SecretKey aesKey = DeriveAESKey(masterSeed, slateId);
	SecureVector decrypted = AES256::Decrypt(encrypted, aesKey, iv);

	ByteBuffer byteBuffer(std::vector<unsigned char>{ decrypted.begin(), decrypted.end() });
	return SlateContextEntity::Deserialize(byteBuffer);
}

Hash SlateContextTable::DeriveAESKey(const SecureVector& masterSeed, const uuids::uuid& slateId)
{
	Serializer serializer;
	serializer.AppendByteVector((const std::vector<unsigned char>&)masterSeed);
	serializer.AppendVarStr(uuids::to_string(slateId));
	serializer.AppendVarStr("context");

	return Hasher::Blake2b(serializer.GetBytes());
}