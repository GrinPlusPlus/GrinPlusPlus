#pragma once

#include "PruneList.h"
#include "MMRHashUtil.h"

#include <string>
#include <Crypto/Models/Hash.h>
#include <Roaring.h>
#include <filesystem.h>
#include <Core/File/BitmapFile.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Core/Serialization/Serializer.h>
#include <PMMR/Common/LeafIndex.h>

#include <filesystem.h>

class LeafSet
{
public:
	using Ptr = std::shared_ptr<LeafSet>;
	using CPtr = std::shared_ptr<const LeafSet>;

	LeafSet(const fs::path& path, const std::shared_ptr<BitmapFile>& pBitmap)
		: m_path(path), m_pBitmap(pBitmap) { }

	static std::shared_ptr<LeafSet> Load(const fs::path& path)
	{
		auto pBitmapFile = BitmapFile::Load(path);

		return std::make_shared<LeafSet>(path, BitmapFile::Load(path));
	}

	void Add(const LeafIndex& leafIndex) { m_pBitmap->Set(leafIndex.Get()); }
	void Remove(const LeafIndex& leafIndex) { m_pBitmap->Unset(leafIndex.Get()); }
	bool Contains(const LeafIndex& leafIndex) const { return m_pBitmap->IsSet(leafIndex.Get()); }

	void Rewind(const uint64_t numLeaves, const std::vector<uint64_t>& leavesToAdd) { m_pBitmap->Rewind(numLeaves, leavesToAdd); }
	void Commit() { m_pBitmap->Commit(); }
	void Rollback() noexcept { m_pBitmap->Rollback(); }
	void Snapshot(const Hash& blockHash)
	{
		GrinStr pathStr = m_path.u8string() + "." + HASH::ShortHash(blockHash);
		Roaring snapshotBitmap = m_pBitmap->ToRoaring();

		const size_t numBytes = snapshotBitmap.getSizeInBytes();
		std::vector<uint8_t> bytes(numBytes);
		const size_t bytesWritten = snapshotBitmap.write((char*)bytes.data());
		if (bytesWritten != numBytes)
		{
			throw std::exception(); // TODO: Handle this.
		}

		FileUtil::SafeWriteToFile(pathStr.ToPath(), bytes);
	}

	Hash Root(const uint64_t numOutputs) const
	{
		const fs::path path = fs::temp_directory_path() / "UBMT";
		std::shared_ptr<HashFile> pHashFile = HashFile::Load(path);
		pHashFile->Rewind(0);

		size_t index = 0;
		std::vector<uint8_t> bytes(128);
		const uint64_t numChunks = (numOutputs + 1023) / 1024;
		for (size_t i = 0; i < numChunks; i++)
		{
			for (size_t j = 0; j < 128; j++)
			{
				bytes[j] = m_pBitmap->GetByte(index++);
			}

			MMRHashUtil::AddHashes(pHashFile, bytes, nullptr);
		}

		return MMRHashUtil::Root(pHashFile, pHashFile->GetSize(), nullptr);
	}

private:
	fs::path m_path;
	std::shared_ptr<BitmapFile> m_pBitmap;
};
