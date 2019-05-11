#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Common/ImportExport.h>
#include <Database/BlockDb.h>
#include <Database/PeerDB.h>
#include <Core/Models/BlockHeader.h>
#include <Crypto/BigInteger.h>

#include <vector>
#include <memory>

// Forward Declarations
class Config;

#ifdef MW_DATABASE
#define DATABASE_API EXPORT
#else
#define DATABASE_API IMPORT
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
	// Opens all node databases and returns an instance of IDatabase.
	//
	DATABASE_API IDatabase* OpenDatabase(const Config& config);

	//
	// Closes all node databases and cleans up the memory of IDatabase.
	//
	DATABASE_API void CloseDatabase(IDatabase* pDatabase);
}