#include "SlateTable.h"
#include "../SqliteDB.h"

#include <Common/Util/StringUtil.h>
#include <Crypto/AES256.h>
#include <Crypto/CSPRNG.h>
#include <Crypto/Hasher.h>
#include <Common/Logger.h>
#include <Wallet/WalletDB/WalletStoreException.h>

void SlateTable::CreateTable(SqliteDB& database)
{
	std::string table_creation_cmd = "create table slate(slate_id TEXT NOT NULL, stage TEXT NOT NULL, iv BLOB NOT NULL, slate BLOB NOT NULL);";
	// TODO: Add indices
	database.Execute(table_creation_cmd);
}

void SlateTable::UpdateSchema(SqliteDB& database, const int previousVersion)
{
	if (previousVersion < 2) {
		CreateTable(database);
	}
}

void SlateTable::SaveSlate(
	SqliteDB& database,
	const SecureVector& masterSeed,
	const Slate& slate)
{
	// TODO: Update if it already exists?
	std::string insert_slate_cmd = "insert into slate values(?, ?, ?, ?)";

	CBigInteger<16> iv(CSPRNG::GenerateRandomBytes(16).data());
	std::vector<uint8_t> encrypted = Encrypt(masterSeed, iv, slate);

	std::vector<SqliteDB::IParameter::UPtr> parameters;
	parameters.push_back(TextParameter::New(uuids::to_string(slate.slateId)));
	parameters.push_back(TextParameter::New(slate.stage.ToString()));
	parameters.push_back(BlobParameter::New(iv.GetData()));
	parameters.push_back(BlobParameter::New(encrypted));
	
	database.Update(insert_slate_cmd, parameters);
}

std::unique_ptr<Slate> SlateTable::LoadSlate(
	SqliteDB& database,
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const SlateStage& stage)
{
	std::string get_slate_query = StringUtil::Format("SELECT iv, slate FROM slate WHERE slate_id='{}' and stage='{}'", uuids::to_string(slateId), stage.ToString());
	auto pStatement = database.Query(get_slate_query);

	if (!pStatement->Step()) {
		WALLET_INFO_F("Slate not found for id {}", uuids::to_string(slateId));
		return nullptr;
	}

	WALLET_DEBUG_F("Slate found for id {}", uuids::to_string(slateId));

	std::vector<uint8_t> iv_bytes = pStatement->GetColumnBytes(0);
	if (iv_bytes.size() != 16) {
		WALLET_ERROR_F("Slate corrupted: {}", database.GetError());
		throw WALLET_STORE_EXCEPTION("Slate corrupted.");
	}

	CBigInteger<16> iv(std::move(iv_bytes));

	try
	{
		std::vector<uint8_t> encrypted = pStatement->GetColumnBytes(1);

		return std::make_unique<Slate>(Decrypt(masterSeed, slateId, iv, encrypted));
	}
	catch (std::exception& e)
	{
		WALLET_ERROR_F("Failed to load slate {}: {}", uuids::to_string(slateId), e);
		throw WALLET_STORE_EXCEPTION("Failed to load slate");
	}
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
	SecretKey aes_key = DeriveAESKey(masterSeed, slate.slateId);

	return AES256::Encrypt(serializer.GetSecureBytes(), aes_key, iv);
}

Slate SlateTable::Decrypt(
	const SecureVector& masterSeed,
	const uuids::uuid& slateId,
	const CBigInteger<16>& iv,
	const std::vector<uint8_t>& encrypted)
{
	SecretKey aesKey = DeriveAESKey(masterSeed, slateId);
	SecureVector decrypted = AES256::Decrypt(encrypted, aesKey, iv);

	ByteBuffer byteBuffer(std::vector<uint8_t>{ decrypted.data(), decrypted.data() + decrypted.size() });
	return Slate::Deserialize(byteBuffer);
}

Hash SlateTable::DeriveAESKey(const SecureVector& masterSeed, const uuids::uuid& slateId)
{
	Serializer serializer;
	serializer.AppendByteVector((const std::vector<unsigned char>&)masterSeed);
	serializer.AppendVarStr(uuids::to_string(slateId));
	serializer.AppendVarStr("slate");

	return Hasher::Blake2b(serializer.GetBytes());
}