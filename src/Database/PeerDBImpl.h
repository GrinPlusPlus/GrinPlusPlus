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
	~PeerDB();

	static std::shared_ptr<PeerDB> OpenDB(const Config& config);

	virtual std::vector<PeerPtr> LoadAllPeers() const override final;
	virtual std::optional<PeerPtr> GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const override final;

	virtual void SavePeers(const std::vector<PeerPtr>& peers) override final;

	virtual void Commit() override final {} // FUTURE: Handle this
	virtual void Rollback() override final {} // FUTURE: Handle this

private:
	PeerDB(const Config& config, DB* pDatabase);

	const Config& m_config;

	DB* m_pDatabase;
};