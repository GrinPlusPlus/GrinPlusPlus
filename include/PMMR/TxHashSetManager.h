#pragma once

#include <Common/ImportExport.h>
#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <Database/BlockDb.h>

#ifdef MW_PMMR
#define TXHASHSET_API EXPORT
#else
#define TXHASHSET_API IMPORT
#endif

class TXHASHSET_API TxHashSetManager
{
public:
	TxHashSetManager(const Config& config, IBlockDB& blockDB);
	~TxHashSetManager() = default;

	ITxHashSet* Open(const BlockHeader& confirmedTip);
	void Flush();
	void Close();

	ITxHashSet* GetTxHashSet();
	const ITxHashSet* GetTxHashSet() const;
	void SetTxHashSet(ITxHashSet* pTxHashSet);
	static void DestroyTxHashSet(ITxHashSet* pTxHashSet);

	static ITxHashSet* LoadFromZip(const Config& config, IBlockDB& blockDB, const std::string& zipFilePath, const BlockHeader& header);
	bool SaveSnapshot(const BlockHeader& header, const std::string& zipFilePath);

private:
	const Config& m_config;
	IBlockDB& m_blockDB;
	ITxHashSet* m_pTxHashSet;

	// TODO: Needs mutex
};