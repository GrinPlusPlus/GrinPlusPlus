#pragma once

#include <Core/File/MappedFile.h>
#include <filesystem.h>
#include <cstdint>
#include <string>
#include <vector>

class AppendOnlyFile
{
public:
	AppendOnlyFile(const fs::path& path)
		: m_path(path),
		m_bufferIndex(0),
		m_fileSize(0),
		m_pMappedFile(nullptr) { }

	AppendOnlyFile(const AppendOnlyFile& file) = delete;
	AppendOnlyFile(AppendOnlyFile&& file) = delete;
	AppendOnlyFile& operator=(const AppendOnlyFile&) = delete;

	virtual ~AppendOnlyFile() = default;

	void Load();
	bool Flush();

	void Append(const std::vector<unsigned char>& data);
	bool Rewind(const uint64_t nextPosition);

	void Discard() noexcept;
	uint64_t GetSize() const noexcept;

	bool Read(
		const uint64_t position,
		const uint64_t numBytes,
		std::vector<unsigned char>& data
	) const;

private:
	fs::path m_path;
	uint64_t m_bufferIndex;
	uint64_t m_fileSize;
	std::vector<unsigned char> m_buffer;
	IMappedFile::UPtr m_pMappedFile;
};