#pragma once

#include <Core/File.h>
#include <Hash.h>
#include <string>

class HashFile
{
public:
	HashFile(const std::string& path);

	bool Load();
	bool Flush();
	bool Rewind(const uint64_t size);
	bool Discard();

	uint64_t GetSize() const;
	Hash GetHashAt(const uint64_t mmrIndex) const;
	
	void AddHash(const Hash& hash);
	void AddHashes(const std::vector<Hash>& hashes);

	Hash Root(const uint64_t size) const;

private:
	// TODO: Store peaks in memory
	File m_file;
};