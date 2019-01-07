#pragma once

#include <Database/Database.h>

#include "BlockDBImpl.h"
#include "PeerDBImpl.h"

class Database : public IDatabase
{
public:
	Database(const Config& config);
	virtual ~Database();

	virtual IBlockDB& GetBlockDB() override final;
	virtual IPeerDB& GetPeerDB() override final;

private:
	const Config& m_config;

	BlockDB m_blockDB;
	PeerDB m_peerDB;
};