#pragma once

#include <Hash.h>
#include <Core/BlockHeader.h>

#include <map>
#include <mutex>

class OrphanPool
{
public:
	bool IsOrphan(const Hash& hash) const { return m_orphanHeaders.count(hash) > 0; }
	void AddOrphan(const BlockHeader& header) { m_orphanHeaders[header.Hash()] = new BlockHeader(header); }

private:
	std::map<Hash, BlockHeader*> m_orphanHeaders;
};