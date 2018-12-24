#include "PeerDBImpl.h"

#include <Infrastructure/Logger.h>
#include <HexUtil.h>
#include <StringUtil.h>
#include <utility>
#include <string>
#include <filesystem>

PeerDB::PeerDB(const Config& config)
	: m_config(config)
{

}

PeerDB::~PeerDB()
{

}

void PeerDB::OpenDB()
{
	//m_dbEnv = lmdb::env::create();

	//// The current windows implementation allocates the whole object at once, so we don't want this too large.
	//// 10 MB should be more than enough to store all the recent peers.
	//m_dbEnv.set_mapsize((size_t)10 * ((size_t)1024UL * 1024UL));

	//// Open "PEERS" environment (lmdb file)
	//const std::string dbPath = m_config.GetDatabaseDirectory() + "PEERS/";
	//std::filesystem::create_directories(dbPath);
	//m_dbEnv.open(dbPath.c_str(), 0, 0664);

	//// Open the default database. Create it if it doesn't yet exist.
	//auto rtxn = lmdb::txn::begin(m_dbEnv);
	//m_dbi = lmdb::dbi::open(rtxn, nullptr, MDB_CREATE);
	//rtxn.commit();
}

void PeerDB::CloseDB()
{
	//m_dbEnv.close();
}

std::vector<Peer> PeerDB::LoadAllPeers()
{
	LoggerAPI::LogInfo("PeerDB::LoadAllPeers - Loading all peers.");

	std::lock_guard<std::mutex> lockGuard(m_mutex);

	// TODO: Implement

	return std::vector<Peer>();
}

std::unique_ptr<Peer> PeerDB::GetPeer(const IPAddress& address)
{
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// TODO: Implement

	return std::unique_ptr<Peer>(nullptr);
}

void PeerDB::AddPeers(const std::vector<Peer>& peers)
{
	std::lock_guard<std::mutex> lockGuard(m_mutex);
	// TODO: Implement
}