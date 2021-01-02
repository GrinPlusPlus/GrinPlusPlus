#pragma once

// Copyright (c) 2018-2019 David Burkett
// Distributed under the MIT software license, see the accompanying
// file LICENSE or http://www.opensource.org/licenses/mit-license.php.

#include <Core/Traits/Lockable.h>

// Forward Declarations
class Config;
class IBlockDB;
class IPeerDB;

#define DATABASE_API

//
// Entrypoint for the Database module.
// Use DatabaseAPI::OpenDatabase to retrieve an instance of IDatabase.
//
class IDatabase
{
public:
	virtual ~IDatabase() = default;

	//
	// The IBlockDB is responsible for managing chain-related storage such as FullBlocks, BlockHeaders, and BlockSums.
	//
	virtual std::shared_ptr<Locked<IBlockDB>> GetBlockDB() = 0;
	virtual std::shared_ptr<const Locked<IBlockDB>> GetBlockDB() const = 0;

	//
	// The IPeerDB is responsible for managing storage of information about recent peers.
	//
	virtual std::shared_ptr<Locked<IPeerDB>> GetPeerDB() = 0;
	virtual std::shared_ptr<const Locked<IPeerDB>> GetPeerDB() const = 0;
};

typedef std::shared_ptr<IDatabase> IDatabasePtr;
typedef std::shared_ptr<const IDatabase> IDatabaseConstPtr;

namespace DatabaseAPI
{
	//
	// Opens all node databases and returns an instance of IDatabase.
	// Caller must call CloseDatabase when finished.
	//
	// Throws DatabaseException if errors occur.
	//
	DATABASE_API IDatabasePtr OpenDatabase(const Config& config);
}