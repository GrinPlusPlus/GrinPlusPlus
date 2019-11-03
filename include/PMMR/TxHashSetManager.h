#pragma once

#include <Common/ImportExport.h>
#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <Database/BlockDb.h>
#include <Core/Traits/Lockable.h>

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

	ITxHashSetPtr Open(const BlockHeader& confirmedTip);
	void Flush();
	void Close();

	ITxHashSetPtr GetTxHashSet();
	ITxHashSetConstPtr GetTxHashSet() const;
	void SetTxHashSet(ITxHashSetPtr pTxHashSet);

	static ITxHashSetPtr LoadFromZip(const Config& config, std::shared_ptr<Locked<IBlockDB>> pDatabase, const std::string& zipFilePath, const BlockHeader& header);
	bool SaveSnapshot(std::shared_ptr<const IBlockDB> pBlockDB, const BlockHeader& header, const std::string& zipFilePath);

private:
	const Config& m_config;
	ITxHashSetPtr m_pTxHashSet;

	// TODO: Needs mutex
};

typedef std::shared_ptr<TxHashSetManager> TxHashSetManagerPtr;
typedef std::shared_ptr<TxHashSetManager> TxHashSetManagerConstPtr;