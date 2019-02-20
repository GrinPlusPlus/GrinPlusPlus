#pragma once

#include "PruneList.h"

#include <string>
#include <Crypto/Hash.h>
#include <Core/CRoaring/roaring.hh>

class LeafSet
{
public:
	LeafSet(const std::string& path);

	void Add(const uint32_t position);
	void Remove(const uint32_t position);
	bool Contains(const uint64_t position) const;

	bool Load();
	void Rewind(const uint64_t size, const Roaring& positionsToAdd);
	bool Flush();
	void Discard();
	bool Snapshot(const Hash& blockHash);

	Roaring CalculatePrunedPositions(const uint64_t cutoffSize, const Roaring& rewindRmPos, const PruneList& pruneList) const;

private:
	Roaring CalculateUnprunedPositions(const uint64_t cutoffSize, const PruneList& pruneList) const;

	bool Flush(const std::string& path, Roaring& bitmap) const;

	const std::string m_path;
	Roaring m_bitmap;
	Roaring m_bitmapBackup;
};