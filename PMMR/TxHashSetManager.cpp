#include <PMMR/TxHashSetManager.h>

#include "TxHashSetImpl.h"
#include "Zip/TxHashSetZip.h"

#include <FileUtil.h>
#include <StringUtil.h>
#include <Infrastructure/Logger.h>

TxHashSetManager::TxHashSetManager(const Config& config, IBlockDB& blockDB)
	: m_config(config), m_blockDB(blockDB), m_pTxHashSet(nullptr)
{

}

ITxHashSet* TxHashSetManager::Open()
{
	Close();

	KernelMMR* pKernelMMR = KernelMMR::Load(m_config);
	OutputPMMR* pOutputPMMR = OutputPMMR::Load(m_config, m_blockDB);
	RangeProofPMMR* pRangeProofPMMR = RangeProofPMMR::Load(m_config);

	m_pTxHashSet = new TxHashSet(m_blockDB, pKernelMMR, pOutputPMMR, pRangeProofPMMR, m_config.GetEnvironment().GetGenesisBlock().GetBlockHeader());

	return m_pTxHashSet;
}

void TxHashSetManager::Flush()
{
	if (m_pTxHashSet != nullptr)
	{
		m_pTxHashSet->Commit();
	}
}

ITxHashSet* TxHashSetManager::LoadFromZip(const Config& config, IBlockDB& blockDB, const std::string& zipFilePath, const BlockHeader& blockHeader)
{
	const TxHashSetZip zip(config);
	if (zip.Extract(zipFilePath, blockHeader))
	{
		LoggerAPI::LogInfo(StringUtil::Format("TxHashSetAPI::LoadFromZip - %s extracted successfully.", zipFilePath.c_str()));
		FileUtil::RemoveFile(zipFilePath);

		KernelMMR* pKernelMMR = KernelMMR::Load(config);
		pKernelMMR->Rewind(blockHeader.GetKernelMMRSize());
		pKernelMMR->Flush();

		OutputPMMR* pOutputPMMR = OutputPMMR::Load(config, blockDB);
		pOutputPMMR->Rewind(blockHeader.GetOutputMMRSize(), Roaring());
		pOutputPMMR->Flush();

		RangeProofPMMR* pRangeProofPMMR = RangeProofPMMR::Load(config);
		pRangeProofPMMR->Rewind(blockHeader.GetOutputMMRSize(), Roaring());
		pRangeProofPMMR->Flush();

		return new TxHashSet(blockDB, pKernelMMR, pOutputPMMR, pRangeProofPMMR, blockHeader);
	}

	return nullptr;
}

ITxHashSet* TxHashSetManager::GetTxHashSet()
{
	return m_pTxHashSet;
}

const ITxHashSet* TxHashSetManager::GetTxHashSet() const
{
	return m_pTxHashSet;
}

void TxHashSetManager::SetTxHashSet(ITxHashSet* pTxHashSet)
{
	Close();
	m_pTxHashSet = pTxHashSet;
}

void TxHashSetManager::DestroyTxHashSet(ITxHashSet* pTxHashSet)
{
	delete pTxHashSet;
}

void TxHashSetManager::Close()
{
	delete m_pTxHashSet;
	m_pTxHashSet = nullptr;
}