#include "HeaderMMRImpl.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Common/Logger.h>
#include <Core/Serialization/Serializer.h>
#include <Core/Config.h>

HeaderMMR::HeaderMMR(std::shared_ptr<Locked<HashFile>> pHashFile)
	: m_pLockedHashFile(pHashFile)
{

}

std::shared_ptr<HeaderMMR> HeaderMMR::Load(const fs::path& path)
{
	std::shared_ptr<HashFile> pHashFile = HashFile::Load(path);
	auto locked = std::make_shared<Locked<HashFile>>(pHashFile);
	return std::make_shared<HeaderMMR>(HeaderMMR(locked));
}

void HeaderMMR::Commit()
{
	if (IsDirty())
	{
		const uint64_t height = Index::At(m_batchDataOpt.value().hashFile->GetSize()).GetLeafIndex();
		LOG_TRACE_F("Flushing - Height: {}, Size: {}", height, m_batchDataOpt.value().hashFile->GetSize());
		m_batchDataOpt.value().hashFile->Commit();
		SetDirty(false);
	}
}

void HeaderMMR::Rollback() noexcept
{
	if (IsDirty())
	{
		LOG_DEBUG("Discarding changes.");
		m_batchDataOpt.value().hashFile->Rollback();
		SetDirty(false);
	}
}

void HeaderMMR::Rewind(const uint64_t size)
{
	const uint64_t mmrSize = LeafIndex::At(size).GetPosition();
	if (mmrSize != m_batchDataOpt.value().hashFile->GetSize())
	{
		LOG_DEBUG_F("Rewinding to height {} - {} hashes", size, mmrSize);
		m_batchDataOpt.value().hashFile->Rewind(mmrSize);
		SetDirty(true);
	}
}

void HeaderMMR::AddHeader(const BlockHeader& header)
{
	assert(m_batchDataOpt.has_value());
	Writer<HashFile> hash_file = m_batchDataOpt.value().hashFile;

	LOG_TRACE_F(
		"Adding header at height {} - MMR size {}",
		header.GetHeight(),
		hash_file->GetSize()
	);

	// Serialize header
	Serializer serializer;
	header.GetProofOfWork().SerializeCycle(serializer);

	// Add hashes
	MMRHashUtil::AddHashes(hash_file.GetShared(), serializer.vec(), nullptr);
	SetDirty(true);
}

Hash HeaderMMR::Root(const uint64_t lastHeight) const
{
	std::shared_ptr<const HashFile> hash_file = m_batchDataOpt.has_value() ?
		m_batchDataOpt.value().hashFile.GetShared() :
		m_pLockedHashFile->Read().GetShared();

	uint64_t position = LeafIndex::At(lastHeight + 1).GetPosition();
	return MMRHashUtil::Root(hash_file, position, nullptr);
}

namespace HeaderMMRAPI
{
	PMMR_API std::shared_ptr<Locked<IHeaderMMR>> OpenHeaderMMR(const Config& config)
	{
		std::shared_ptr<IHeaderMMR> pHeaderMMR = HeaderMMR::Load(config.GetChainPath() / "header_mmr.bin");
		return std::make_shared<Locked<IHeaderMMR>>(pHeaderMMR);
	}
}