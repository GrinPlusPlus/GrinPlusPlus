#include "PeerDBImpl.h"

#include <Infrastructure/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <utility>
#include <string>
#include <filesystem.h>

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
	options.max_open_files = 10;
	
	//options.OptimizeLevelStyleCompaction();
	// create the DB if it's not already present
	options.create_if_missing = true;
	options.compression = kNoCompression;

	// open DB
	const std::string dbPath = m_config.GetDatabaseDirectory() + "PEERS/";
	fs::create_directories(dbPath);

	Status s = DB::Open(options, dbPath, &m_pDatabase); // TODO: Define columns (Peer by address, Peer by capabilities, Peer by last contact, etc.)?
}

void PeerDB::CloseDB()
{
	delete m_pDatabase;
}

std::vector<Peer> PeerDB::LoadAllPeers()
{
	LoggerAPI::LogTrace("PeerDB::LoadAllPeers - Loading all peers.");

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

std::optional<Peer> PeerDB::GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt)
{
	LoggerAPI::LogTrace("PeerDB::LoadAllPeers - Loading peer: " + (portOpt.has_value() ? SocketAddress(address, portOpt.value()).Format() : address.Format()));

	Serializer addressSerializer;
	if (address.IsLocalhost() && portOpt.has_value())
	{
		SocketAddress(address, portOpt.value()).Serialize(addressSerializer);
	}
	else
	{
		address.Serialize(addressSerializer);
	}

	Slice key((const char*)addressSerializer.GetBytes().data(), addressSerializer.GetBytes().size());

	std::string value;
	const Status status = m_pDatabase->Get(ReadOptions(), key, &value);
	if (status.ok())
	{
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(data);

		return std::make_optional<Peer>(Peer::Deserialize(byteBuffer));
	}

	return std::nullopt;
}

void PeerDB::SavePeers(const std::vector<Peer>& peers)
{
	LoggerAPI::LogTrace("PeerDB::SavePeers - Saving peers: " + std::to_string(peers.size()));

	WriteBatch writeBatch;
	for (const Peer& peer : peers)
	{
		const IPAddress& address = peer.GetIPAddress();

		Serializer addressSerializer;
		address.Serialize(addressSerializer);
		Slice key((const char*)addressSerializer.GetBytes().data(), addressSerializer.GetBytes().size());

		Serializer peerSerializer;
		peer.Serialize(peerSerializer);
		Slice value((const char*)peerSerializer.GetBytes().data(), peerSerializer.GetBytes().size());

		writeBatch.Put(key, value);
	}

	m_pDatabase->Write(WriteOptions(), &writeBatch);
}