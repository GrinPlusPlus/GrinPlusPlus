#include "DatabaseImpl.h"

Database::Database(const Config& config)
	: m_config(config), m_blockDB(config), m_peerDB(config)
{
	m_blockDB.OpenDB();
	m_peerDB.OpenDB();
}

Database::~Database()
{
	m_peerDB.CloseDB();
	m_blockDB.CloseDB();
}

IBlockDB& Database::GetBlockDB()
{
	return m_blockDB;
}

IPeerDB& Database::GetPeerDB()
{
	return m_peerDB;
}

namespace DatabaseAPI
{
	//
	// Creates a new instance of the BlockChain server.
	//
	DATABASE_API IDatabase* OpenDatabase(const Config& config)
	{
		IDatabase* pDatabase = new Database(config);
		return pDatabase;
	}

	//
	// Stops the BlockChain server and clears up its memory usage.
	//
	DATABASE_API void CloseDatabase(IDatabase* pDatabase)
	{
		Database* pDatabaseImpl = (Database*)pDatabase;
		delete pDatabaseImpl;
		pDatabase = nullptr;
	}
}