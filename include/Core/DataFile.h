#pragma once

#include <Core/AppendOnlyFile.h>
#include <Core/Exceptions/FileException.h>
#include <Core/Traits/Batchable.h>
#include <Crypto/BigInteger.h>
#include <Common/Util/StringUtil.h>
#include <memory>

template<size_t NUM_BYTES>
class DataFile : public Traits::IBatchable
{
public:
	static std::shared_ptr<DataFile> Load(const std::string& path)
	{ 
		std::shared_ptr<AppendOnlyFile> pFile = std::make_shared<AppendOnlyFile>(path);
		pFile->Load();

		return std::make_shared<DataFile>(DataFile(pFile));
	}

	virtual void Commit() override final
	{
		if (IsDirty())
		{
			if (!m_pFile->Flush())
			{
				throw FILE_EXCEPTION("Commit failed.");
			}
		}

		SetDirty(false);
	}

	virtual void Rollback() override final
	{
		if (IsDirty())
		{
			if (!m_pFile->Discard())
			{
				throw FILE_EXCEPTION("Rollback failed.");
			}
		}

		SetDirty(false);
	}

	void Rewind(const uint64_t size)
	{
		SetDirty(true);
		if (!m_pFile->Rewind(size * NUM_BYTES))
		{
			throw FILE_EXCEPTION(StringUtil::Format("Failed to rewind to size %llu", size));
		}
	}

	uint64_t GetSize() const
	{
		return m_pFile->GetSize() / NUM_BYTES;
	}

	std::vector<unsigned char> GetDataAt(const uint64_t position) const
	{
		std::vector<unsigned char> data;
		if (!m_pFile->Read(position * NUM_BYTES, NUM_BYTES, data))
		{
			throw FILE_EXCEPTION(StringUtil::Format("Failed to read data at position %llu", position));
		}

		return data;
	}

	void AddData(const std::vector<unsigned char>& data)
	{
		SetDirty(true);
		m_pFile->Append(data);
	}

	void AddData(const CBigInteger<NUM_BYTES>& data)
	{
		SetDirty(true);
		m_pFile->Append(data.GetData());
	}

private:
	DataFile(std::shared_ptr<AppendOnlyFile> pFile)
		: m_pFile(pFile)
	{

	}

	std::shared_ptr<AppendOnlyFile> m_pFile;
};