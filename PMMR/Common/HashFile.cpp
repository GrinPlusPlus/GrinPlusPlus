#include "HashFile.h"
#include "MMRUtil.h"
#include "MMRHashUtil.h"

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