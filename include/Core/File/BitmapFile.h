#pragma once

#include <Core/Traits/Batchable.h>
#include <Roaring.h>
#include <Common/Util/BitUtil.h>
#include <fstream>
#include <functional>
#include <algorithm>
#include <map>
#include <memory>

#ifdef _WIN32
#define MPATH_STR m_path.wstring()
#else
#define MPATH_STR m_path.string()
#endif

// NOTE: Uses bit positions numbered from 0-7, starting at the left.
// For example, 65 (01000001) has bit positions 1 and 7 set.
class BitmapFile : public Traits::IBatchable
{
public:
	virtual ~BitmapFile() = default;

	static std::shared_ptr<BitmapFile> Load(const fs::path& path)
	{
		auto pBitmapFile = std::shared_ptr<BitmapFile>(new BitmapFile(path));
		pBitmapFile->Load();
		return pBitmapFile;
	}

	static std::shared_ptr<BitmapFile> Create(const fs::path& path, const Roaring& bitmap)
	{
		auto pBitmapFile = Load(path);
		pBitmapFile->Set(bitmap);
		pBitmapFile->Commit();

		return pBitmapFile;
	}

	virtual void Commit() override final
	{
		if (!m_modifiedBytes.empty())
		{
			m_mmap.unmap();

			std::ofstream file(m_path.c_str(), std::ios_base::binary | std::ios_base::out | std::ios_base::in);

			for (auto iter : m_modifiedBytes)
			{
				file.seekp(iter.first);
				file.write((const char*)&iter.second, 1);
			}

			file.close();

			std::error_code error;
			m_mmap = mio::make_mmap_source(MPATH_STR, error);
			if (error.value() != 0)
			{
				// TODO: Handle this
			}

			m_modifiedBytes.clear();
			SetDirty(false);
		}
	}

	virtual void Rollback() override final
	{
		m_modifiedBytes.clear();
		SetDirty(false);
	}

	bool IsSet(const uint64_t position) const
	{
		return GetByte(position / 8) & BitToByte(position % 8);
	}

	void Set(const uint64_t position)
	{
		SetDirty(true);
		uint8_t byte = GetByte(position / 8);
		byte |= BitToByte(position % 8);
		m_modifiedBytes[position / 8] = byte;
	}

	void Set(const Roaring& positionsToSet)
	{
		for (auto iter = positionsToSet.begin(); iter != positionsToSet.end(); iter++)
		{
			Set(iter.i.current_value - 1);
		}
	}

	void Unset(const uint64_t position)
	{
		SetDirty(true);
		uint8_t byte = GetByte(position / 8);
		byte &= (0xff ^ BitToByte(position % 8));
		m_modifiedBytes[position / 8] = byte;
	}

	void Unset(const Roaring& positionsToUnset)
	{
		for (auto iter = positionsToUnset.begin(); iter != positionsToUnset.end(); iter++)
		{
			Unset(iter.i.current_value - 1);
		}
	}

	const bool& operator[] (const size_t position) const
	{
		return IsSet(position) ? s_true : s_false;
	}

	void Rewind(const size_t size, const Roaring& positionsToAdd)
	{
		Set(positionsToAdd);

		size_t currentSize = GetNumBytes() * 8;
		for (size_t i = size; i < currentSize; i++)
		{
			Unset(i);
		}
	}

	Roaring ToRoaring() const
	{
		Roaring bitmap;

		const uint64_t numBytes = GetNumBytes();
		for (uint32_t i = 0; i < (uint32_t)numBytes; i++)
		{
			const uint8_t byte = GetByte(i);
			for (uint8_t j = 0; j < 8; j++)
			{
				if ((byte & BitToByte(j)) > 0)
				{
					bitmap.add((i * 8) + j + 1);
				}
			}
		}

		return bitmap;
	}

private:
	BitmapFile(const fs::path& path) : m_path(path), m_size(0) { }

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

		m_size = FileUtil::GetFileSize(m_path);

		if (m_size > 0)
		{
			std::error_code error;
			m_mmap = mio::make_mmap_source(MPATH_STR, error);
			if (error.value() != 0)
			{
				LOG_ERROR_F("Failed to mmap file: {}", error.value());
				throw FILE_EXCEPTION_F("Failed to mmap file: {}", m_path);
			}
		}
	}

	uint8_t GetByte(const uint64_t byteIndex) const
	{
		auto iter = m_modifiedBytes.find(byteIndex);
		if (iter != m_modifiedBytes.cend())
		{
			return iter->second;
		}
		else if (byteIndex < m_mmap.size())
		{
			return *((uint8_t*)(m_mmap.cbegin() + byteIndex));
		}

		return 0;
	}

	uint64_t GetNumBytes() const
	{
		size_t size = 0;
		if (!m_modifiedBytes.empty())
		{
			size = m_modifiedBytes.crbegin()->first + 1;
		}

		if (!m_mmap.empty())
		{
			size = (std::max)(size, m_mmap.size() + 1);
		}

		return size;
	}

	// Returns a byte with the given bit (0-7) set.
	// Example: BitToByte(2) returns 32 (00100000).
	uint8_t BitToByte(const uint8_t bit) const
	{
		return 1 << (7 - bit);
	}

	bool IsBitSet(const uint8_t byte, const uint64_t position) const
	{
		const uint8_t bitPosition = (uint8_t)position % 8;

		return (byte >> (7 - bitPosition)) & 1;
	}

	fs::path m_path;
	std::map<uint64_t, uint8_t> m_modifiedBytes;
	mio::mmap_source m_mmap;
	uint64_t m_size;

	static const bool s_true{ false };
	static const bool s_false{ false };
};