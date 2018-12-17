#pragma once

#include "CRoaring/roaring.hh"
#include "PruneList.h"

#include <string>
#include <Hash.h>

class LeafSet
{
public:
	LeafSet(const std::string& path);

	void Add(const uint32_t position);
	void Remove(const uint32_t position);
	bool Contains(const uint32_t position) const;

	bool Load();
	bool Flush();
	bool Snapshot(const Hash& blockHash);
	void DiscardChanges();

	Roaring CalculatePrunedPositions(const uint64_t cutoffSize, const Roaring& rewindRmPos, const PruneList& pruneList) const;

private:
	Roaring CalculateUnprunedPositions(const uint64_t cutoffSize, const PruneList& pruneList) const;

	bool Flush(const std::string& path, Roaring& bitmap) const;

	const std::string m_path;
	Roaring m_bitmap;
	Roaring m_bitmapBackup; // TODO: Use a batch/transaction object with a commit(), instead
};