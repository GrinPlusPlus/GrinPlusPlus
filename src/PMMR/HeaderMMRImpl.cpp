#include "HeaderMMRImpl.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Infrastructure/Logger.h>
#include <Core/Serialization/Serializer.h>
#include <Config/Config.h>

HeaderMMR::HeaderMMR(std::shared_ptr<Locked<HashFile>> pHashFile)
	: m_pLockedHashFile(pHashFile)
{

}

std::shared_ptr<HeaderMMR> HeaderMMR::Load(const std::string& path)
{
	std::shared_ptr<HashFile> pHashFile = HashFile::Load(path);
	auto locked = std::make_shared<Locked<HashFile>>(pHashFile);
	return std::make_shared<HeaderMMR>(HeaderMMR(locked));
}

void HeaderMMR::Commit()
{
	const uint64_t height = MMRUtil::GetNumLeaves(m_batchDataOpt.value().hashFile->GetSize());
	LOG_TRACE_F("Flushing - Height: %llu, Size: %llu", height, m_batchDataOpt.value().hashFile->GetSize());
	m_batchDataOpt.value().hashFile->Commit();
	SetDirty(false);
}

void HeaderMMR::Rollback()
{
	LOG_DEBUG("Discarding changes.");
	m_batchDataOpt.value().hashFile->Rollback();
	SetDirty(false);
}

bool HeaderMMR::Rewind(const uint64_t size)
{
	const uint64_t mmrSize = MMRUtil::GetNumNodes(MMRUtil::GetPMMRIndex(size - 1));
	if (mmrSize != m_batchDataOpt.value().hashFile->GetSize())
	{
		LOG_DEBUG_F("Rewinding to height %llu - %llu hashes", size, mmrSize);
		m_batchDataOpt.value().hashFile->Rewind(mmrSize);
	}

	return true;
}

void HeaderMMR::AddHeader(const BlockHeader& header)
{
	LOG_TRACE_F("Adding header at height %llu - MMR size %llu", header.GetHeight(), m_batchDataOpt.value().hashFile->GetSize());

	// Serialize header
	Serializer serializer;
	header.GetProofOfWork().SerializeProofNonces(serializer);
	const std::vector<unsigned char> serializedHeader = serializer.GetBytes();

	// Add hashes
	MMRHashUtil::AddHashes(m_batchDataOpt.value().hashFile.GetShared(), serializedHeader, nullptr);
}

Hash HeaderMMR::Root(const uint64_t lastHeight) const
{
	const uint64_t position = MMRUtil::GetNumNodes(MMRUtil::GetPMMRIndex(lastHeight));

	if (m_batchDataOpt.has_value())
	{
		return MMRHashUtil::Root(m_batchDataOpt.value().hashFile.GetShared(), position, nullptr);
	}
	else
	{
		return MMRHashUtil::Root(m_pLockedHashFile->Read().GetShared(), position, nullptr);
	}
}

namespace HeaderMMRAPI
{
	PMMR_API std::shared_ptr<Locked<IHeaderMMR>> OpenHeaderMMR(const Config& config)
	{
		std::shared_ptr<IHeaderMMR> pHeaderMMR = HeaderMMR::Load(config.GetChainDirectory() + "header_mmr.bin");
		return std::make_shared<Locked<IHeaderMMR>>(pHeaderMMR);
	}
}