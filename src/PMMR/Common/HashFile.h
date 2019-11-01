#pragma once

#include <Core/DataFile.h>
#include <Crypto/Hash.h>
#include <string>

typedef DataFile<32> HashFile;

//class HashFileTransaction : public FileTransaction<32>
//{
//public:
//	HashFileTransaction(File& file) : FileTransaction<32>(file) { }
//
//	inline Hash GetHashAt(const uint64_t mmrIndex) const
//	{
//		std::vector<unsigned char> data;
//		if (GetDataAt(mmrIndex, data))
//		{
//			return Hash(std::move(data));
//		}
//
//		return ZERO_HASH;
//	}
//
//	inline void AddHash(const Hash& hash)
//	{
//		AddData(hash.GetData());
//	}
//};
//
//class HashFile : public DataFile<32>
//{
//public:
//	HashFile(const std::string& path) : DataFile<32>(path) { };
//
//	std::shared_ptr<HashFileTransaction> StartTransaction()
//	{
//		return std::make_shared<HashFileTransaction>(*this);
//	}
//
//	//inline Hash GetHashAt(const uint64_t mmrIndex) const
//	//{
//	//	std::vector<unsigned char> data;
//	//	if (GetDataAt(mmrIndex, data))
//	//	{
//	//		return Hash(std::move(data));
//	//	}
//
//	//	return ZERO_HASH;
//	//}
//	//
//	//inline void AddHash(const Hash& hash)
//	//{
//	//	AddData(hash.GetData());
//	//}
//};