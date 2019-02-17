#pragma once

#include <Core/File.h>

template<size_t NUM_BYTES>
class DataFile
{
public:
	DataFile(const std::string& path)
		: m_file(path)
	{

	}

	inline bool Load()
	{ 
		return m_file.Load();
	}

	inline bool Flush()
	{
		return m_file.Flush();
	}

	inline bool Rewind(const uint64_t size)
	{
		return m_file.Rewind(size * NUM_BYTES);
	}

	inline bool Discard()
	{
		return m_file.Discard();
	}

	inline uint64_t GetSize() const
	{
		return m_file.GetSize() / NUM_BYTES;
	}

	inline bool GetDataAt(const uint64_t position, std::vector<unsigned char>& data) const
	{
		return m_file.Read(position * NUM_BYTES, NUM_BYTES, data);
	}

	inline void AddData(const std::vector<unsigned char>& data)
	{
		m_file.Append(data);
	}

private:
	File m_file;
};