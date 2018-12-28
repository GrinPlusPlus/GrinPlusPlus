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
	Options options;
	// Optimize RocksDB. This is the easiest way to get RocksDB to perform well
	options.IncreaseParallelism();
	//options.OptimizeLevelStyleCompaction();
	// create the DB if it's not already present
	options.create_if_missing = true;
	options.compression = kNoCompression;

	// open DB
	const std::string dbPath = m_config.GetDatabaseDirectory() + "PEERS/";
	std::filesystem::create_directories(dbPath);

	Status s = DB::Open(options, dbPath, &m_pDatabase); // TODO: Define columns (Peer by address, Peer by capabilities, Peer by last contact, etc.)?
}

void PeerDB::CloseDB()
{
	//delete m_pDatabase;
}

std::vector<Peer> PeerDB::LoadAllPeers()
{
	LoggerAPI::LogInfo("PeerDB::LoadAllPeers - Loading all peers.");

	std::vector<Peer> peers;

	rocksdb::Iterator* it = m_pDatabase->NewIterator(rocksdb::ReadOptions());
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		std::vector<unsigned char> data(it->value().data(), it->value().data() + it->value().size());
		ByteBuffer byteBuffer(data);
		peers.emplace_back(Peer::Deserialize(byteBuffer));
	}

	return peers;
}

std::unique_ptr<Peer> PeerDB::GetPeer(const IPAddress& address)
{
	// TODO: Implement

	return std::unique_ptr<Peer>(nullptr);
}

void PeerDB::AddPeers(const std::vector<Peer>& peers)
{
	LoggerAPI::LogInfo("PeerDB::AddPeers - Adding peers - " + std::to_string(peers.size()));

	for (const Peer& peer : peers)
	{
		const IPAddress& address = peer.GetIPAddress();

		Serializer addressSerializer;
		address.Serialize(addressSerializer);
		Slice key((const char*)addressSerializer.GetBytes().data(), addressSerializer.GetBytes().size());

		Serializer peerSerializer;
		peer.Serialize(peerSerializer);
		Slice value((const char*)peerSerializer.GetBytes().data(), peerSerializer.GetBytes().size());

		m_pDatabase->Put(WriteOptions(), key, value);
	}
}