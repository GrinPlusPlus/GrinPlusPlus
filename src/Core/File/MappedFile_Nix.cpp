#include "MappedFile_Nix.h"

#include <fstream>
#include <filesystem.h>
#include <stdlib.h>
#include <Core/Exceptions/FileException.h>
#include <Common/Logger.h>
#include <Common/Util/FileUtil.h>

MappedFile::~MappedFile()
{
	LOG_TRACE_F("Closing File: {}", m_path);

	Unmap();
}

IMappedFile::UPtr IMappedFile::Load(const fs::path& path)
{
	std::ifstream inFile(path, std::ios::in | std::ifstream::ate | std::ifstream::binary);
	if (inFile.is_open())
	{
		inFile.close();
	}
	else
	{
		LOG_INFO_F("File {} does not exist. Creating it now.", path);
		std::ofstream outFile(path, std::ios::out | std::ios::binary | std::ios::app);
		if (!outFile.is_open())
		{
			LOG_ERROR_F("Failed to create file: {}", path);
			throw FILE_EXCEPTION_F("Failed to create file: {}", path);
		}

		outFile.close();
	}

	return std::unique_ptr<IMappedFile>(new MappedFile(path));
}

bool MappedFile::Write(const size_t startIndex, const std::vector<uint8_t>& data)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	Unmap();

	FileUtil::TruncateFile(m_path, startIndex);

	if (!data.empty())
	{
		std::ofstream file(m_path, std::ios::out | std::ios::binary | std::ios::app);
		if (!file.is_open())
		{
			LOG_ERROR_F("Failed to open file: {}", m_path);
			return false;
		}

		file.seekp(startIndex, std::ios::beg);
		file.write((const char*)data.data(), data.size());
		file.close();
	}

	return true;
}

void MappedFile::Read(const uint64_t position, const uint64_t numBytes, std::vector<uint8_t>& data) const
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (!m_mmap.is_mapped())
	{
		Map();
	}

	data = std::vector<uint8_t>(
		m_mmap.cbegin() + position,
		m_mmap.cbegin() + position + numBytes
	);
}

void MappedFile::Map() const
{
	std::error_code error;
	m_mmap = mio::make_mmap_source(m_path.c_str(), error);
	if (error.value() != 0)
	{
		LOG_ERROR_F("Failed to mmap file: {}", error.value());
		throw FILE_EXCEPTION_F("Failed to mmap file: {}", m_path);
	}
}

void MappedFile::Unmap() const
{
	m_mmap.unmap();
}