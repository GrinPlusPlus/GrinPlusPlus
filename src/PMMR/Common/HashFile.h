#pragma once

#include <Core/DataFile.h>
#include <Crypto/Hash.h>
#include <string>

class HashFile : public DataFile<32>
{
public:
	HashFile(const std::string& path) : DataFile<32>(path) { };

	inline Hash GetHashAt(const uint64_t mmrIndex) const
	{
		std::vector<unsigned char> data;
		if (GetDataAt(mmrIndex, data))
		{
			return Hash(std::move(data));
		}

		return ZERO_HASH;
	}
	
	inline void AddHash(const Hash& hash)
	{
		AddData(hash.GetData());
	}
};