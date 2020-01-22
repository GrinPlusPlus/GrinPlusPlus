#pragma once

#include <Common/ImportExport.h>
#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <Database/BlockDb.h>
#include <Core/Traits/Lockable.h>
#include <filesystem.h>

#ifdef MW_PMMR
#define TXHASHSET_API EXPORT
#else
#define TXHASHSET_API IMPORT
#endif

class TXHASHSET_API TxHashSetManager
{
public:
	TxHashSetManager(const Config& config);
	~TxHashSetManager() = default;

	std::shared_ptr<Locked<ITxHashSet>> Open(BlockHeaderPtr pConfirmedTip);
	void Close() { m_pTxHashSet.reset(); }

	std::shared_ptr<Locked<ITxHashSet>> GetTxHashSet() { return m_pTxHashSet; }
	std::shared_ptr<const Locked<ITxHashSet>> GetTxHashSet() const { return m_pTxHashSet; }
	void SetTxHashSet(ITxHashSetPtr pTxHashSet) { m_pTxHashSet = std::make_shared<Locked<ITxHashSet>>(Locked<ITxHashSet>(pTxHashSet)); }

	static ITxHashSetPtr LoadFromZip(const Config& config, const fs::path& zipFilePath, BlockHeaderPtr pHeader);
	fs::path SaveSnapshot(std::shared_ptr<const IBlockDB> pBlockDB, BlockHeaderPtr pHeader);

private:
	const NodeConfig& m_config;
	std::shared_ptr<Locked<ITxHashSet>> m_pTxHashSet;

	// TODO: Needs mutex
};

typedef std::shared_ptr<TxHashSetManager> TxHashSetManagerPtr;
typedef std::shared_ptr<TxHashSetManager> TxHashSetManagerConstPtr;