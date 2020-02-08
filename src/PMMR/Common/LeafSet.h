#pragma once

#include "PruneList.h"
#include "MMRUtil.h"
#include "MMRHashUtil.h"

#include <string>
#include <Crypto/Hash.h>
#include <Roaring.h>
#include <filesystem.h>
#include <Core/File/BitmapFile.h>
#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Core/Serialization/Serializer.h>

#include <filesystem.h>

class LeafSet
{
public:
	static std::shared_ptr<LeafSet> Load(const fs::path& path)
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
		std::string path = m_path.u8string() + "." + HASH::ShortHash(blockHash);
		Roaring snapshotBitmap = m_pBitmap->ToRoaring();

		const size_t numBytes = snapshotBitmap.getSizeInBytes();
		std::vector<unsigned char> bytes(numBytes);
		const size_t bytesWritten = snapshotBitmap.write((char*)bytes.data());
		if (bytesWritten != numBytes)
		{
			throw std::exception(); // TODO: Handle this.
		}

		FileUtil::SafeWriteToFile(FileUtil::ToPath(path), bytes);
	}

	Hash Root(const uint64_t numOutputs)
	{
		const fs::path path = fs::temp_directory_path() / "UBMT";
		std::shared_ptr<HashFile> pHashFile = HashFile::Load(path);
		pHashFile->Rewind(0);

		size_t index = 0;
		//const uint64_t numLeaves = MMRUtil::GetNumLeaves(bitmap.maximum());
		const uint64_t numChunks = (numOutputs + 1023) / 1024;
		for (size_t i = 0; i < numChunks; i++)
		{
			Serializer serializer;
			for (size_t j = 0; j < (1024 / 8); j++)
			{
				uint8_t power = 128;
				uint8_t byte = 0;
				for (size_t k = 0; k < 8; k++)
				{
					if (index < numOutputs && m_pBitmap->IsSet(MMRUtil::GetPMMRIndex(index)))
					{
						byte += power;
					}

					power /= 2;
					++index;
				}

				serializer.Append(byte);
			}

			MMRHashUtil::AddHashes(pHashFile, serializer.GetBytes(), nullptr);
		}

		return MMRHashUtil::Root(pHashFile, pHashFile->GetSize(), nullptr);
	}

private:
	LeafSet(const fs::path& path, std::shared_ptr<BitmapFile> pBitmap)
		: m_path(path), m_pBitmap(pBitmap)
	{

	}

	fs::path m_path;
	std::shared_ptr<BitmapFile> m_pBitmap;
};
