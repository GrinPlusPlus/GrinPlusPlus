#include <PMMR/TxHashSetManager.h>

#include "TxHashSetImpl.h"
#include "Zip/TxHashSetZip.h"
#include "Zip/Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

TxHashSetManager::TxHashSetManager(const Config& config, IBlockDB& blockDB)
	: m_config(config), m_blockDB(blockDB), m_pTxHashSet(nullptr)
{

}

ITxHashSet* TxHashSetManager::Open(const BlockHeader& confirmedTip)
{
	Close();

	KernelMMR* pKernelMMR = KernelMMR::Load(m_config.GetTxHashSetDirectory());
	OutputPMMR* pOutputPMMR = OutputPMMR::Load(m_config.GetTxHashSetDirectory(), m_blockDB);
	RangeProofPMMR* pRangeProofPMMR = RangeProofPMMR::Load(m_config.GetTxHashSetDirectory());

	m_pTxHashSet = new TxHashSet(m_blockDB, pKernelMMR, pOutputPMMR, pRangeProofPMMR, confirmedTip);

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

		KernelMMR* pKernelMMR = KernelMMR::Load(config.GetTxHashSetDirectory());
		pKernelMMR->Rewind(blockHeader.GetKernelMMRSize());
		pKernelMMR->Flush();

		OutputPMMR* pOutputPMMR = OutputPMMR::Load(config.GetTxHashSetDirectory(), blockDB);
		pOutputPMMR->Rewind(blockHeader.GetOutputMMRSize(), std::nullopt);
		pOutputPMMR->Flush();

		RangeProofPMMR* pRangeProofPMMR = RangeProofPMMR::Load(config.GetTxHashSetDirectory());
		pRangeProofPMMR->Rewind(blockHeader.GetOutputMMRSize(), std::nullopt);
		pRangeProofPMMR->Flush();

		return new TxHashSet(blockDB, pKernelMMR, pOutputPMMR, pRangeProofPMMR, blockHeader);
	}

	return nullptr;
}

bool TxHashSetManager::SaveSnapshot(const BlockHeader& blockHeader, const std::string& zipFilePath)
{
	if (m_pTxHashSet == nullptr)
	{
		return false;
	}

	// 1. Lock TxHashSet
	TxHashSet* pTxHashSet = (TxHashSet*)m_pTxHashSet;
	pTxHashSet->ReadLock();

	// 2. Copy to Snapshots/Hash // TODO: If already exists, just use that.
	const std::string txHashSetDirectory = m_config.GetTxHashSetDirectory();

	const std::string snapshotDir = fs::temp_directory_path().string() + "Snapshots/" + blockHeader.FormatHash() + "/";
	if (!FileUtil::CopyDirectory(txHashSetDirectory, snapshotDir))
	{
		pTxHashSet->Unlock();
		return false;
	}

	const BlockHeader flushedBlockHeader = pTxHashSet->GetFlushedBlockHeader();

	// 3. Unlock TxHashSet
	pTxHashSet->Unlock();

	{
		// 4. Load Snapshot TxHashSet
		KernelMMR* pKernelMMR = KernelMMR::Load(snapshotDir);
		OutputPMMR* pOutputPMMR = OutputPMMR::Load(snapshotDir, m_blockDB);
		RangeProofPMMR* pRangeProofPMMR = RangeProofPMMR::Load(snapshotDir);
		TxHashSet snapshotTxHashSet(m_blockDB, pKernelMMR, pOutputPMMR, pRangeProofPMMR, flushedBlockHeader);

		// 5. Rewind Snapshot TxHashSet
		if (!snapshotTxHashSet.Rewind(blockHeader))
		{
			return false;
		}

		// 6. Flush Snapshot TxHashSet
		if (!snapshotTxHashSet.Commit())
		{
			return false;
		}

		// 7. Rename pmmr_leaf files
		FileUtil::RenameFile(snapshotDir + "output/pmmr_leaf.bin", snapshotDir + "output/pmmr_leaf.bin." + blockHeader.FormatHash());
		FileUtil::RenameFile(snapshotDir + "rangeproof/pmmr_leaf.bin", snapshotDir + "rangeproof/pmmr_leaf.bin." + blockHeader.FormatHash());

		// 8. Create Zip
		const std::vector<std::string> pathsToZip = { snapshotDir + "kernel", snapshotDir + "output", snapshotDir + "rangeproof" };
		if (!Zipper::CreateZipFile(zipFilePath, pathsToZip))
		{
			return false;
		}
	}

	// 9. Delete Snapshots/Hash folder
	FileUtil::RemoveFile(snapshotDir);

	return true;
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
	delete (TxHashSet*)pTxHashSet;
}

void TxHashSetManager::Close()
{
	delete (TxHashSet*)m_pTxHashSet;
	m_pTxHashSet = nullptr;
}