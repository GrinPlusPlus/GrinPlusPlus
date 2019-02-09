#include "HeaderMMRImpl.h"
#include "Common/MMRUtil.h"

#include <Infrastructure/Logger.h>
#include <Serialization/Serializer.h>
#include <Crypto.h>
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
	LoggerAPI::LogDebug("HeaderMMR::Commit - Flushing. Height:" + std::to_string(height) + " Size: " + std::to_string(m_hashFile.GetSize()));
	return m_hashFile.Flush();
}

bool HeaderMMR::Rewind(const uint64_t size)
{
	const uint64_t mmrSize = MMRUtil::GetNumNodes(MMRUtil::GetPMMRIndex(size - 1));
	if (mmrSize != m_hashFile.GetSize())
	{
		LoggerAPI::LogDebug("HeaderMMR::Rewind - Rewinding to height " + std::to_string(size) + " Hashes: " + std::to_string(mmrSize));
		return m_hashFile.Rewind(mmrSize);
	}

	return true;
}

bool HeaderMMR::Rollback()
{
	LoggerAPI::LogDebug("HeaderMMR::Rollback - Discarding changes.");
	return m_hashFile.Discard();
}

void HeaderMMR::AddHeader(const BlockHeader& header)
{
	LoggerAPI::LogTrace("HeaderMMR::AddHeader - Adding header at height " + std::to_string(header.GetHeight()) + " MMR: " + std::to_string(m_hashFile.GetSize()));

	uint64_t position = m_hashFile.GetSize();
	uint64_t peak = 1;

	// Add in the new leaf
	m_hashFile.AddHash(HashWithIndex(header, position));

	// Add parents
	while (MMRUtil::GetHeight(position + 1) > 0)
	{
		const uint64_t leftSiblingPosition = (position + 1) - (2 * peak);

		const Hash leftHash = m_hashFile.GetHashAt(leftSiblingPosition);
		const Hash rightHash = m_hashFile.GetHashAt(position);

		++position;
		peak *= 2;

		const Hash parentHash = MMRUtil::HashParentWithIndex(leftHash, rightHash, position);
		m_hashFile.AddHash(parentHash);
	}
}

Hash HeaderMMR::Root(const uint64_t lastHeight) const
{
	const uint64_t position = MMRUtil::GetNumNodes(MMRUtil::GetPMMRIndex(lastHeight));

	return m_hashFile.Root(position);
}

Hash HeaderMMR::HashWithIndex(const BlockHeader& header, const uint64_t index) const
{
	Serializer serializer;
	serializer.Append<uint64_t>(index);
	header.GetProofOfWork().SerializeProofNonces(serializer);
	return Crypto::Blake2b(serializer.GetBytes());
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