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
	virtual std::optional<Peer> GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) override final;

	virtual void SavePeers(const std::vector<Peer>& peers) override final;

private:
	const Config& m_config;

	DB* m_pDatabase;
};