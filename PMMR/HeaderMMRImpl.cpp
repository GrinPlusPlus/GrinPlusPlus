#include "HeaderMMRImpl.h"
#include "Common/MMRUtil.h"
#include "Common/MMRHashUtil.h"

#include <Infrastructure/Logger.h>
#include <Core/Serialization/Serializer.h>
#include <Config/Config.h>

HeaderMMR::HeaderMMR(const std::string& path)
	: m_hashFile(path)
{

}

bool HeaderMMR::Load()
{
	return m_hashFile.Load();
}

bool HeaderMMR::Commit()
{
	const uint64_t height = MMRUtil::GetNumLeaves(m_hashFile.GetSize());
	LOG_TRACE_F("Flushing - Height: %llu, Size: %llu", height, m_hashFile.GetSize());
	return m_hashFile.Flush();
}

bool HeaderMMR::Rewind(const uint64_t size)
{
	const uint64_t mmrSize = MMRUtil::GetNumNodes(MMRUtil::GetPMMRIndex(size - 1));
	if (mmrSize != m_hashFile.GetSize())
	{
		LOG_DEBUG_F("Rewinding to height %llu - %llu hashes", size, mmrSize);
		return m_hashFile.Rewind(mmrSize);
	}

	return true;
}

bool HeaderMMR::Rollback()
{
	LOG_DEBUG("Discarding changes.");
	return m_hashFile.Discard();
}

void HeaderMMR::AddHeader(const BlockHeader& header)
{
	LOG_TRACE_F("Adding header at height %llu - MMR size %llu", header.GetHeight(), m_hashFile.GetSize());

	// Serialize header
	Serializer serializer;
	header.GetProofOfWork().SerializeProofNonces(serializer);
	const std::vector<unsigned char> serializedHeader = serializer.GetBytes();

	// Add hashes
	MMRHashUtil::AddHashes(m_hashFile, serializedHeader, nullptr);
}

Hash HeaderMMR::Root(const uint64_t lastHeight) const
{
	const uint64_t position = MMRUtil::GetNumNodes(MMRUtil::GetPMMRIndex(lastHeight));

	return MMRHashUtil::Root(m_hashFile, position, nullptr);
}

namespace HeaderMMRAPI
{
	PMMR_API IHeaderMMR* OpenHeaderMMR(const Config& config)
	{
		HeaderMMR* pHeaderMMR = new HeaderMMR(config.GetChainDirectory() + "header_mmr.bin");
		pHeaderMMR->Load();

		return pHeaderMMR;
	}

	PMMR_API void CloseHeaderMMR(IHeaderMMR* pIHeaderMMR)
	{
		HeaderMMR* pHeaderMMR = (HeaderMMR*)pIHeaderMMR;
		pHeaderMMR->Commit();

		delete pHeaderMMR;
	}
}