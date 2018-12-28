#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

#include <Database/PeerDB.h>
#include <Config/Config.h>
#include <mutex>

using namespace rocksdb;

class PeerDB : public IPeerDB 
{
public:
	PeerDB(const Config& config);
	~PeerDB();

	void OpenDB();
	void CloseDB();

	virtual std::vector<Peer> LoadAllPeers() override final;
	virtual std::unique_ptr<Peer> GetPeer(const IPAddress& address) override final;

	virtual void AddPeers(const std::vector<Peer>& peers) override final;

private:
	const Config& m_config;

	DB* m_pDatabase;
};