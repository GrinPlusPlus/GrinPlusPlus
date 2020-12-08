#include "MappedFile_Win.h"

#include <fstream>
#include <vector>
#include <filesystem.h>
#include <stdlib.h>
#include <Core/Exceptions/FileException.h>
#include <Common/Logger.h>

MappedFile::~MappedFile()
{
	//LOG_TRACE_F("Closing File: {}", m_path);

	Unmap();

	if (m_handle != INVALID_HANDLE_VALUE)
	{
#ifdef _WIN32
		//SetEndOfFile(m_handle);
		CloseHandle(m_handle);
#endif
	}
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

	mio::file_handle_type handle = CreateFile(
		path.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (handle == INVALID_HANDLE_VALUE)
	{
		LOG_ERROR_F("Failed to open file: {}", path);
		throw FILE_EXCEPTION_F("Failed to open file: {}", path);
	}

	return std::unique_ptr<IMappedFile>(new MappedFile(path, handle));
}

bool MappedFile::Write(const size_t startIndex, const std::vector<uint8_t>& data)
{
	std::unique_lock<std::mutex> lock(m_mutex);

	Unmap();

	LARGE_INTEGER li;
	li.QuadPart = startIndex;

	bool success = SetFilePointerEx(m_handle, li, NULL, FILE_BEGIN);
	if (!success)
	{
		LOG_ERROR_F("Failed to set file pointer for {} - error: {}", m_path, GetLastError());
		return false;
	}

	if (!data.empty())
	{
		DWORD bytesWritten;
		if (FALSE == WriteFile(m_handle, (const char*)data.data(), (DWORD)data.size(), &bytesWritten, 0))
		{
			LOG_ERROR_F("Failed to write to {} - error: {}", m_path, GetLastError());
			return false;
		}
	}

	success = SetEndOfFile(m_handle);
	if (!success)
	{
		LOG_ERROR_F("Failed to set end of file for {} - error: {}", m_path, GetLastError());
		//return false;
	}

	return true;
}

void MappedFile::Read(const uint64_t position, const uint64_t numBytes, std::vector<uint8_t>& data) const
{
	std::unique_lock<std::mutex> lock(m_mutex);

	if (!m_mmap.IsMapped())
	{
		Map();
	}

	data = std::vector<uint8_t>(
		m_mmap.mapped_view + position,
		m_mmap.mapped_view + position + numBytes
	);
}

void MappedFile::Map() const
{
	m_mmap.mapping_handle = CreateFileMapping(m_handle, 0, PAGE_READONLY, 0, 0, 0);
	if (m_mmap.mapping_handle == INVALID_HANDLE_VALUE)
	{
		LOG_ERROR_F("Failed to create file mapping: {}", m_path);
		throw FILE_EXCEPTION_F("Failed to create file mapping: {}", m_path);
	}
	
	m_mmap.mapped_view = (const char*)MapViewOfFile(m_mmap.mapping_handle, FILE_MAP_READ, 0, 0, 0);
	if (m_mmap.mapped_view == nullptr)
	{
		LOG_ERROR_F("Failed to map view of file: {}", m_path);
		throw FILE_EXCEPTION_F("Failed to map view of file: {}", m_path);
	}
}

void MappedFile::Unmap() const
{
	if (m_mmap.IsMapped())
	{
		if (!UnmapViewOfFile(m_mmap.mapped_view))
		{
			LOG_ERROR_F("Failed to unmap view of file: {}", m_path);
			throw FILE_EXCEPTION_F("Failed to unmap view of file: {}", m_path);
		}

		if (!CloseHandle(m_mmap.mapping_handle))
		{
			LOG_ERROR_F("Failed to close file mapping: {}", m_path);
			throw FILE_EXCEPTION_F("Failed to close file mapping: {}", m_path);
		}

		m_mmap.mapped_view = nullptr;
		m_mmap.mapping_handle = INVALID_HANDLE_VALUE;
	}
}