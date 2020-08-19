#include "OutputsTable.h"
#include "../../WalletEncryptionUtil.h"

#include <Common/Logger.h>
#include <Common/Util/StringUtil.h>
#include <Wallet/WalletDB/WalletStoreException.h>

// TABLE: outputs
// id: INTEGER PRIMARY KEY
// commitment: TEXT NOT NULL
// status: INTEGER NOT NULL
// transaction_id: INTEGER
// encrypted: BLOB NOT NULL
void OutputsTable::CreateTable(SqliteDB& database)
{
	std::string table_creation_cmd = "create table outputs(id INTEGER PRIMARY KEY, commitment TEXT UNIQUE NOT NULL, status INTEGER NOT NULL, transaction_id INTEGER, encrypted BLOB NOT NULL);";
	database.Execute(table_creation_cmd);
}

void OutputsTable::UpdateSchema(SqliteDB& database, const SecureVector& masterSeed, const int previousVersion)
{
	if (previousVersion >= 1) {
		return;
	}

	// Create "new_outputs" table
	std::string table_creation_cmd = "create table new_outputs(id INTEGER PRIMARY KEY, commitment TEXT UNIQUE NOT NULL, status INTEGER NOT NULL, transaction_id INTEGER, encrypted BLOB NOT NULL);";
	database.Execute(table_creation_cmd);

	// Load all outputs from existing table
	std::vector<OutputDataEntity> outputs = GetOutputs(database, masterSeed, previousVersion);

	// Add outputs to "new_outputs" table
	AddOutputs(database, masterSeed, outputs, "new_outputs");

	// Delete existing table
	std::string drop_table_cmd = "DROP TABLE outputs";
	database.Execute(drop_table_cmd);

	// Rename "new_outputs" table to "outputs"
	const std::string rename_table_cmd = "ALTER TABLE new_outputs RENAME TO outputs";
	database.Execute(rename_table_cmd);
}

void OutputsTable::AddOutputs(SqliteDB& database, const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs)
{
	AddOutputs(database, masterSeed, outputs, "outputs");
}

void OutputsTable::AddOutputs(SqliteDB& database, const SecureVector& masterSeed, const std::vector<OutputDataEntity>& outputs, const std::string& tableName)
{
	for (const OutputDataEntity& output : outputs)
	{
		WALLET_DEBUG_F("Saving output: {}", output.GetOutput());

		std::string insert_output_cmd = "insert into " + tableName + "(commitment, status, transaction_id, encrypted) values(?, ?, ?, ?)";
		insert_output_cmd += " ON CONFLICT(commitment) DO UPDATE SET status=excluded.status, transaction_id=excluded.transaction_id, encrypted=excluded.encrypted";

		Serializer serializer;
		output.Serialize(serializer);
		std::vector<uint8_t> encrypted = WalletEncryptionUtil::Encrypt(masterSeed, "OUTPUT", serializer.GetSecureBytes());

		std::vector<SqliteDB::IParameter::UPtr> parameters;
		parameters.push_back(std::make_unique<TextParameter>(output.GetOutput().GetCommitment().ToHex()));
		parameters.push_back(std::make_unique<IntParameter>((int)output.GetStatus()));
		
		if (output.GetWalletTxId().has_value()) {
			parameters.push_back(std::make_unique<IntParameter>((int)output.GetWalletTxId().value()));
		} else {
			parameters.push_back(std::make_unique<NullParameter>());
		}

		parameters.push_back(std::make_unique<BlobParameter>(encrypted));

		database.Update(insert_output_cmd, parameters);
	}
}

std::vector<OutputDataEntity> OutputsTable::GetOutputs(SqliteDB& database, const SecureVector& masterSeed)
{
	return GetOutputs(database, masterSeed, 1);
}

std::vector<OutputDataEntity> OutputsTable::GetOutputs(SqliteDB& database, const SecureVector& masterSeed, const int /*version*/)
{
	// Prepare statement
	std::string get_encrypted_query = "select encrypted from outputs";
	auto pStatement = database.Query(get_encrypted_query);

	std::vector<OutputDataEntity> outputs;
	while (pStatement->Step())
	{
		std::vector<uint8_t> encrypted = pStatement->GetColumnBytes(0);
		SecureVector decrypted = WalletEncryptionUtil::Decrypt(masterSeed, "OUTPUT", encrypted);
		std::vector<uint8_t> decryptedUnsafe(decrypted.begin(), decrypted.end());

		ByteBuffer byteBuffer(std::move(decryptedUnsafe));
		outputs.emplace_back(OutputDataEntity::Deserialize(byteBuffer));
	}

	return outputs;
}