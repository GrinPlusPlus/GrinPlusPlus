#pragma once

#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4267)
#pragma warning(disable:4334)
#pragma warning(disable:4018)
#include <mio/mmap.hpp>
#pragma warning(pop)

#include <Core/Exceptions/FileException.h>
#include <Infrastructure/Logger.h>
#include <Common/Util/FileUtil.h>
#include <stdint.h>
#include <string>
#include <vector>

#ifdef _WIN32
#define MPATH_STR m_path.wstring()
#else
#define MPATH_STR m_path.string()
#endif

class AppendOnlyFile
{
public:
	AppendOnlyFile(const std::string& path)
		: m_path(StringUtil::ToWide(path)),
		m_bufferIndex(0),
		m_fileSize(0)
	{

	}

	AppendOnlyFile(const AppendOnlyFile& file) = delete;
	AppendOnlyFile(AppendOnlyFile&& file) = delete;
	AppendOnlyFile& operator=(const AppendOnlyFile&) = delete;

	void Load()
	{
		std::ifstream inFile(m_path.c_str(), std::ios::in | std::ifstream::ate | std::ifstream::binary);
		if (inFile.is_open())
		{
			inFile.close();
		}
		else
		{
			LOG_INFO_F("File {} does not exist. Creating it now.", m_path);
			std::ofstream outFile(m_path.c_str(), std::ios::out | std::ios::binary | std::ios::app);
			if (!outFile.is_open())
			{
				LOG_ERROR_F("Failed to create file: {}", m_path);
				throw FILE_EXCEPTION_F("Failed to create file: {}", m_path);
			}

			outFile.close();
		}

		m_fileSize = FileUtil::GetFileSize(m_path.u8string());
		m_bufferIndex = m_fileSize;

		if (m_fileSize > 0)
		{
			std::error_code error;
			m_mmap = mio::make_mmap_source(MPATH_STR, error);
			if (error.value() > 0)
			{
				LOG_ERROR_F("Failed to mmap file: {}", error.value());
				throw FILE_EXCEPTION_F("Failed to mmap file: {}", m_path);
			}
		}
	}

	bool Flush()
	{
		if (m_fileSize == m_bufferIndex && m_buffer.empty())
		{
			return true;
		}

		if (m_fileSize < m_bufferIndex)
		{
			return false;
		}

		m_mmap.unmap();

		if (m_fileSize > m_bufferIndex)
		{
			FileUtil::TruncateFile(m_path.u8string(), m_bufferIndex);
		}

		if (!m_buffer.empty())
		{
			std::ofstream file(m_path.c_str(), std::ios::out | std::ios::binary | std::ios::app);
			if (!file.is_open())
			{
				return false;
			}

			file.seekp(m_bufferIndex, std::ios::beg);

			if (!m_buffer.empty())
			{
				file.write((const char*)& m_buffer[0], m_buffer.size());
			}

			file.close();
		}

		m_fileSize = m_bufferIndex + m_buffer.size();

		m_bufferIndex = m_fileSize;
		m_buffer.clear();

		if (m_fileSize > 0)
		{
			std::error_code error;
			m_mmap = mio::make_mmap_source(MPATH_STR, error);
			if (error.value() > 0)
			{
				LOG_ERROR_F("Failed to mmap file: {}", error.value());
			}
		}

		return true;
	}

	void Append(const std::vector<unsigned char>& data)
	{
		m_buffer.insert(m_buffer.end(), data.cbegin(), data.cend());
	}

	bool Rewind(const uint64_t nextPosition)
	{
		if (!Flush())
		{
			return false;
		}

		if (nextPosition > m_fileSize)
		{
			return false;
		}

		if (nextPosition <= m_bufferIndex)
		{
			m_buffer.clear();
			m_bufferIndex = nextPosition;
		}
		else
		{
			m_buffer.erase(m_buffer.begin() + nextPosition - m_bufferIndex, m_buffer.end());
		}

		return true;
	}

	bool Discard()
	{
		m_bufferIndex = m_fileSize;
		m_buffer.clear();

		return true;
	}

	uint64_t GetSize() const
	{
		return m_bufferIndex + m_buffer.size();
	}

	bool Read(const uint64_t position, const uint64_t numBytes, std::vector<unsigned char>& data) const
	{
		if (position < m_bufferIndex)
		{
			data = std::vector<unsigned char>(m_mmap.cbegin() + position, m_mmap.cbegin() + position + numBytes);
		}
		else
		{
			const uint64_t firstBufferIndex = position - m_bufferIndex;

			data = std::vector<unsigned char>(m_buffer.cbegin() + firstBufferIndex, m_buffer.cbegin() + firstBufferIndex + numBytes);
		}

		return true;
	}

private:
	fs::path m_path;
	uint64_t m_bufferIndex;
	uint64_t m_fileSize;
	std::vector<unsigned char> m_buffer;
	mio::mmap_source m_mmap;
};