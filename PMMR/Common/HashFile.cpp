#include "HashFile.h"
#include "MMRUtil.h"

#include <Infrastructure/Logger.h>

HashFile::HashFile(const std::string& path)
	: m_file(path)
{

}

bool HashFile::Load()
{
	return m_file.Load();
}

bool HashFile::Rewind(const uint64_t size)
{
	return m_file.Rewind(size * HASH_SIZE);
}

bool HashFile::Discard()
{
	return m_file.Discard();
}

bool HashFile::Flush()
{
	return m_file.Flush();
}

uint64_t HashFile::GetSize() const
{
	return m_file.GetSize() / HASH_SIZE;
}

Hash HashFile::GetHashAt(const uint64_t mmrIndex) const
{
	std::vector<unsigned char> data;
	if (m_file.Read(mmrIndex * HASH_SIZE, HASH_SIZE, data))
	{
		return Hash(std::move(data));
	}

	return ZERO_HASH;
}

void HashFile::AddHash(const Hash& hash)
{
	m_file.Append(hash.GetData());
}

void HashFile::AddHashes(const std::vector<Hash>& hashes)
{
	std::vector<unsigned char> data;
	data.reserve(hashes.size() * HASH_SIZE);
	for (const Hash& hash : hashes)
	{
		data.insert(data.end(), hash.GetData().cbegin(), hash.GetData().cend());
	}

	m_file.Append(data);
}

Hash HashFile::Root(const uint64_t size) const
{
	LoggerAPI::LogTrace("HashFile::Root - Calculating root with size " + std::to_string(size));

	if (size == 0)
	{
		return ZERO_HASH;
	}

	Hash hash = ZERO_HASH;
	const std::vector<uint64_t> peakIndices = MMRUtil::GetPeakIndices(size);
	for (auto iter = peakIndices.crbegin(); iter != peakIndices.crend(); iter++)
	{
		if (hash == ZERO_HASH)
		{
			hash = GetHashAt(*iter);
		}
		else
		{
			hash = MMRUtil::HashParentWithIndex(GetHashAt(*iter), hash, size);
		}
	}

	return hash;
}