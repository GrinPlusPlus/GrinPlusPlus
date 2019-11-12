#pragma once

#include "PruneList.h"

#include <string>
#include <Crypto/Hash.h>
#include <Roaring.h>
#include <Core/BitmapFile.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>

class LeafSet
{
public:
	static std::shared_ptr<LeafSet> Load(const std::string& path)
	{
		auto pBitmapFile = BitmapFile::Load(path);

		return std::shared_ptr<LeafSet>(new LeafSet(path, pBitmapFile));
	}

	void Add(const uint32_t position) { m_pBitmap->Set(position); }
	void Remove(const uint32_t position) { m_pBitmap->Unset(position); }
	bool Contains(const uint64_t position) const { return m_pBitmap->IsSet(position); }

	void Rewind(const uint64_t size, const Roaring& positionsToAdd) { m_pBitmap->Rewind(size, positionsToAdd); }
	void Commit() { m_pBitmap->Commit(); }
	void Rollback() { m_pBitmap->Rollback(); }
	void Snapshot(const Hash& blockHash)
	{
		std::string path = m_path + "." + HexUtil::ShortHash(blockHash);
		Roaring snapshotBitmap = m_pBitmap->ToRoaring();

		const size_t numBytes = snapshotBitmap.getSizeInBytes();
		std::vector<unsigned char> bytes(numBytes);
		const size_t bytesWritten = snapshotBitmap.write((char*)bytes.data());
		if (bytesWritten != numBytes)
		{
			throw std::exception(); // TODO: Handle this.
		}

		if (!FileUtil::SafeWriteToFile(path, bytes))
		{
			throw std::exception(); // TODO: Handle this.
		}
	}

private:
	LeafSet(const std::string& path, std::shared_ptr<BitmapFile> pBitmap)
		: m_path(path), m_pBitmap(pBitmap)
	{

	}

	std::string m_path;
	std::shared_ptr<BitmapFile> m_pBitmap;
};