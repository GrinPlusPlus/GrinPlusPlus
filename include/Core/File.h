#pragma once

#include <stdint.h>
#include <string>
#include <vector>

class File
{
public:
	File(const std::string& path);

	bool Load();
	bool Flush();

	void Append(const std::vector<unsigned char>& data);

	bool Rewind(const uint64_t nextPosition); // TODO: Is this lastPosition or nextPosition?
	bool Discard();

	uint64_t GetSize() const;
	bool Read(const uint64_t position, const uint64_t numBytes, std::vector<unsigned char>& data) const;

private:
	const std::string m_path;
	uint64_t m_bufferIndex;
	uint64_t m_fileSize;
	std::vector<unsigned char> m_buffer;
};