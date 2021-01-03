#include "PeerDBImpl.h"

#include <Database/DatabaseException.h>
#include <Common/Logger.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/StringUtil.h>
#include <utility>
#include <string>
#include <filesystem.h>

PeerDB::PeerDB(const Config& config, DB* pDatabase)
	: m_config(config), m_pDatabase(pDatabase)
{

}

PeerDB::~PeerDB()
{
	delete m_pDatabase;
}

std::shared_ptr<PeerDB> PeerDB::OpenDB(const Config& config)
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
	const fs::path dbPath = config.GetDatabasePath() / "PEERS";
	fs::create_directories(dbPath);

	DB* pDatabase = nullptr;
	Status status = DB::Open(options, dbPath.u8string(), &pDatabase); // TODO: Define columns (Peer by address, Peer by capabilities, Peer by last contact, etc.)?
	if (!status.ok())
	{
		LOG_ERROR_F("DB::Open failed with error: {}", status.getState());
		throw DATABASE_EXCEPTION("DB::Open failed with error: " + std::string(status.getState()));
	}

	return std::shared_ptr<PeerDB>(new PeerDB(config, pDatabase));
}

std::vector<PeerPtr> PeerDB::LoadAllPeers() const
{
	LOG_TRACE("Loading all peers.");

	std::vector<PeerPtr> peers;

	std::unique_ptr<rocksdb::Iterator> it(m_pDatabase->NewIterator(rocksdb::ReadOptions()));
	for (it->SeekToFirst(); it->Valid(); it->Next())
	{
		std::vector<unsigned char> data(it->value().data(), it->value().data() + it->value().size());
		ByteBuffer byteBuffer(std::move(data));
		peers.emplace_back(Peer::Deserialize(byteBuffer));
	}

	return peers;
}

std::optional<PeerPtr> PeerDB::GetPeer(const IPAddress& address, const std::optional<uint16_t>& portOpt) const
{
	LOG_TRACE("Loading peer: " + (portOpt.has_value() ? SocketAddress(address, portOpt.value()).Format() : address.Format()));

	Serializer addressSerializer;
	if (address.IsLocalhost() && portOpt.has_value())
	{
		SocketAddress(address, portOpt.value()).Serialize(addressSerializer);
	}
	else
	{
		address.Serialize(addressSerializer);
	}

	Slice key((const char*)addressSerializer.data(), addressSerializer.size());

	std::string value;
	const Status status = m_pDatabase->Get(ReadOptions(), key, &value);
	if (status.ok())
	{
		std::vector<unsigned char> data(value.data(), value.data() + value.size());
		ByteBuffer byteBuffer(std::move(data));

		return std::make_optional(Peer::Deserialize(byteBuffer));
	}

	return std::nullopt;
}

void PeerDB::SavePeers(const std::vector<PeerPtr>& peers)
{
	LOG_TRACE_F("Saving peers: {}", peers.size());

	WriteBatch writeBatch;
	for (const PeerPtr& peer : peers)
	{
		const IPAddress& address = peer->GetIPAddress();

		Serializer addressSerializer;
		address.Serialize(addressSerializer);
		Slice key((const char*)addressSerializer.data(), addressSerializer.size());

		Serializer peerSerializer;
		peer->Serialize(peerSerializer);
		Slice value((const char*)peerSerializer.data(), peerSerializer.size());

		writeBatch.Put(key, value);
	}

	m_pDatabase->Write(WriteOptions(), &writeBatch);
}

void PeerDB::DeletePeers(const std::vector<PeerPtr>& peers)
{
	LOG_TRACE_F("Deleting peers: {}", peers.size());

	WriteBatch writeBatch;
	for (const PeerPtr& peer : peers)
	{
		const IPAddress& address = peer->GetIPAddress();

		Serializer addressSerializer;
		address.Serialize(addressSerializer);
		Slice key((const char*)addressSerializer.data(), addressSerializer.size());

		writeBatch.Delete(key);
	}

	m_pDatabase->Write(WriteOptions(), &writeBatch);
}