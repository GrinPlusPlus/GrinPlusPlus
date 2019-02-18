#pragma once

#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4334)
#pragma warning(disable:4018)
#include <mio/mmap.hpp>
#pragma warning(pop)

#include <stdint.h>
#include <string>
#include <vector>

class File
{
public:
	File(const std::string& path);

	File(const File& file) = delete;
	File(File&& file) = delete;
	File& operator=(const File&) = delete;

	bool Load();
	bool Flush();

	void Append(const std::vector<unsigned char>& data);

	bool Rewind(const uint64_t nextPosition);
	// TODO: bool Commit(); - Should eliminate a rewind-point, but not actually flush to disk.
	bool Discard();

	uint64_t GetSize() const;
	bool Read(const uint64_t position, const uint64_t numBytes, std::vector<unsigned char>& data) const;

private:
	std::string m_path;
	uint64_t m_bufferIndex;
	uint64_t m_fileSize;
	std::vector<unsigned char> m_buffer;
	mio::mmap_source m_mmap;
};