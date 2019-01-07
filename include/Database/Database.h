#pragma once

//
// This code is free for all purposes without any express guarantee it works.
//
// Author: David Burkett (davidburkett38@gmail.com)
//

#include <ImportExport.h>
#include <Database/BlockDB.h>
#include <Database/PeerDB.h>
#include <Core/BlockHeader.h>
#include <BigInteger.h>

#include <vector>
#include <memory>

// Forward Declarations
class Config;

#ifdef MW_DATABASE
#define DATABASE_API __declspec(dllexport)
#else
#define DATABASE_API __declspec(dllimport)
#endif

class IDatabase
{
public:
	virtual IBlockDB& GetBlockDB() = 0;

	virtual IPeerDB& GetPeerDB() = 0;
};

namespace DatabaseAPI
{
	//
	// Creates a new instance of the BlockChain server.
	//
	DATABASE_API IDatabase* OpenDatabase(const Config& config);

	//
	// Stops the BlockChain server and clears up its memory usage.
	//
	DATABASE_API void CloseDatabase(IDatabase* pDatabase);
}