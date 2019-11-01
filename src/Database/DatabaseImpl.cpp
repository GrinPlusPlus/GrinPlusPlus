#include "DatabaseImpl.h"

#include <Database/DatabaseException.h>

Database::Database(const Config& config, std::shared_ptr<Locked<IBlockDB>> pBlockDB, std::shared_ptr<Locked<IPeerDB>> pPeerDB)
	: m_config(config), m_pBlockDB(pBlockDB), m_pPeerDB(pPeerDB)
{

}

std::shared_ptr<IDatabase> Database::Open(const Config& config)
{
	std::shared_ptr<BlockDB> pBlockDB(new BlockDB(config));
	pBlockDB->OpenDB();

	std::shared_ptr<PeerDB> pPeerDB(new PeerDB(config));
	pPeerDB->OpenDB();

	return std::shared_ptr<IDatabase>(new Database(config, std::make_shared<Locked<IBlockDB>>(pBlockDB), std::make_shared<Locked<IPeerDB>>(pPeerDB)));
}

namespace DatabaseAPI
{
	//
	// Creates a new instance of the BlockChain server.
	//
	DATABASE_API std::shared_ptr<IDatabase> OpenDatabase(const Config& config)
	{
		try
		{
			return Database::Open(config);
		}
		catch (DatabaseException&)
		{
			throw;
		}
		catch (std::exception& e)
		{
			throw DATABASE_EXCEPTION("Exception caught: " + std::string(e.what()));
		}
	}

	////
	//// Stops the BlockChain server and clears up its memory usage.
	////
	//DATABASE_API void CloseDatabase(IDatabase* pDatabase)
	//{
	//	Database* pDatabaseImpl = (Database*)pDatabase;
	//	try
	//	{
	//		pDatabaseImpl->Close();
	//	}
	//	catch (DatabaseException&)
	//	{
	//		delete pDatabaseImpl;
	//		throw;
	//	}
	//	catch (std::exception& e)
	//	{
	//		delete pDatabaseImpl;
	//		throw DATABASE_EXCEPTION("Exception caught: " + std::string(e.what()));
	//	}

	//	delete pDatabaseImpl;
	//}
}