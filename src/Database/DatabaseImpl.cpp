#include "DatabaseImpl.h"

#include <Database/DatabaseException.h>

Database::Database(const Config& config, std::shared_ptr<Locked<IBlockDB>> pBlockDB, std::shared_ptr<Locked<IPeerDB>> pPeerDB)
	: m_config(config), m_pBlockDB(pBlockDB), m_pPeerDB(pPeerDB)
{

}

std::shared_ptr<IDatabase> Database::Open(const Config& config)
{
	std::shared_ptr<BlockDB> pBlockDB = BlockDB::OpenDB(config);
	std::shared_ptr<PeerDB> pPeerDB(PeerDB::OpenDB(config));

	return std::shared_ptr<IDatabase>(new Database(config, std::make_shared<Locked<IBlockDB>>(pBlockDB), std::make_shared<Locked<IPeerDB>>(pPeerDB)));
}

namespace DatabaseAPI
{
	//
	// Creates a new instance of the Database.
	//
	DATABASE_API std::shared_ptr<IDatabase> OpenDatabase(const Config& config)
	{
		try
		{
			return Database::Open(config);
		}
		catch (DatabaseException& e)
		{
			LOG_ERROR_F("Failed to open database with error: {}", e.what());
			throw;
		}
		catch (std::exception& e)
		{
			throw DATABASE_EXCEPTION("Exception caught: " + std::string(e.what()));
		}
	}
}