#pragma once

#include <ImportExport.h>
#include <PMMR/TxHashSet.h>
#include <Config/Config.h>
#include <Database/BlockDb.h>

#ifdef MW_PMMR
#define TXHASHSET_API __declspec(dllexport)
#else
#define TXHASHSET_API __declspec(dllimport)
#endif

class TXHASHSET_API TxHashSetManager
{
public:
	TxHashSetManager(const Config& config, IBlockDB& blockDB);
	~TxHashSetManager() = default;

	ITxHashSet* Open();
	void Flush();
	void Close();

	ITxHashSet* GetTxHashSet();
	const ITxHashSet* GetTxHashSet() const;
	void SetTxHashSet(ITxHashSet* pTxHashSet);
	static void DestroyTxHashSet(ITxHashSet* pTxHashSet);

	static ITxHashSet* LoadFromZip(const Config& config, IBlockDB& blockDB, const std::string& zipFilePath, const BlockHeader& header);

private:
	const Config& m_config;
	IBlockDB& m_blockDB;
	ITxHashSet* m_pTxHashSet;

	// TODO: Needs mutex
};