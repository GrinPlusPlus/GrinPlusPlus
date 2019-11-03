#pragma once

#include "PruneList.h"

#include <string>
#include <Crypto/Hash.h>
#include <CRoaring/roaring.hh>

class LeafSet
{
public:
	static std::shared_ptr<LeafSet> Load(const std::string& path);

	void Add(const uint32_t position);
	void Remove(const uint32_t position);
	bool Contains(const uint64_t position) const;
	const Roaring& GetBitmap() const { return m_bitmap; }

	void Rewind(const uint64_t size, const Roaring& positionsToAdd);
	bool Flush();
	void Discard();
	bool Snapshot(const Hash& blockHash);

	Roaring CalculatePrunedPositions(const uint64_t cutoffSize, const Roaring& rewindRmPos, const PruneList& pruneList) const;

private:
	LeafSet(const std::string& path, const Roaring& bitmap);

	Roaring CalculateUnprunedPositions(const uint64_t cutoffSize, const PruneList& pruneList) const;

	bool Flush(const std::string& path, Roaring& bitmap) const;

	const std::string m_path;
	Roaring m_bitmap;
	Roaring m_bitmapBackup;
};