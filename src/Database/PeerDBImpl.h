#pragma once

#include <rocksdb/db.h>
#include <rocksdb/slice.h>
#include <rocksdb/options.h>

#include <Database/PeerDB.h>
#include <Core/Config.h>
#include <mutex>

using namespace rocksdb;

class PeerDB : public IPeerDB
{
public:
	virtual ~PeerDB();

	static std::shared_ptr<PeerDB> OpenDB(const Config& config);

	std::vector<PeerPtr> LoadAllPeers() const final;
	std::optional<PeerPtr> GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const final;

	void SavePeers(const std::vector<PeerPtr>& peers) final;
	void DeletePeers(const std::vector<PeerPtr>& peers) final;

	void Commit() final {} // FUTURE: Handle this
	void Rollback() noexcept final {} // FUTURE: Handle this

private:
	PeerDB(const Config& config, DB* pDatabase);

	const Config& m_config;

	DB* m_pDatabase;
};