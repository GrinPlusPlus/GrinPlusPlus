#include <PMMR/TxHashSetManager.h>

#include "TxHashSetImpl.h"
#include "Zip/TxHashSetZip.h"
#include "Zip/Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

#include <filesystem.h>

TxHashSetManager::TxHashSetManager(const Config& config)
	: m_config(config), m_pTxHashSet(nullptr)
{

}

std::shared_ptr<Locked<ITxHashSet>> TxHashSetManager::Open(BlockHeaderPtr pConfirmedTip)
{
	Close();

	std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(m_config.GetNodeConfig().GetTxHashSetPath());
	std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(m_config.GetNodeConfig().GetTxHashSetPath());
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(m_config.GetNodeConfig().GetTxHashSetPath());

	auto pTxHashSet = std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, pConfirmedTip));
	m_pTxHashSet = std::make_shared<Locked<ITxHashSet>>(Locked<ITxHashSet>(pTxHashSet));

	return m_pTxHashSet;
}

std::shared_ptr<ITxHashSet> TxHashSetManager::LoadFromZip(const Config& config, const fs::path& zipFilePath, BlockHeaderPtr pHeader)
{
	const TxHashSetZip zip(config);
	if (zip.Extract(zipFilePath, *pHeader))
	{
		LOG_INFO_F("{} extracted successfully", zipFilePath);
		FileUtil::RemoveFile(zipFilePath.u8string());

		// Rewind Kernel MMR
		std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(config.GetNodeConfig().GetTxHashSetPath());
		pKernelMMR->Rewind(pHeader->GetKernelMMRSize());
		pKernelMMR->Commit();

		// Create output BitmapFile from Roaring file
		const std::string leafPath = config.GetNodeConfig().GetTxHashSetPath().u8string() + "output/pmmr_leaf.bin";
		Roaring outputBitmap;
		std::vector<unsigned char> outputBytes;
		if (FileUtil::ReadFile(leafPath, outputBytes))
		{
			outputBitmap = Roaring::readSafe((const char*)outputBytes.data(), outputBytes.size());
		}
		BitmapFile::Create(config.GetNodeConfig().GetTxHashSetPath().u8string() + "output/pmmr_leafset.bin", outputBitmap);

		// Rewind Output MMR
		std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(config.GetNodeConfig().GetTxHashSetPath());
		pOutputPMMR->Rewind(pHeader->GetOutputMMRSize(), Roaring());
		pOutputPMMR->Commit();

		// Create rangeproof BitmapFile from Roaring file
		const std::string rangeproofPath = config.GetNodeConfig().GetTxHashSetPath().u8string() + "rangeproof/pmmr_leaf.bin";
		Roaring rangeproofBitmap;
		std::vector<unsigned char> rangeproofBytes;
		if (FileUtil::ReadFile(rangeproofPath, rangeproofBytes))
		{
			rangeproofBitmap = Roaring::readSafe((const char*)rangeproofBytes.data(), rangeproofBytes.size());
		}
		BitmapFile::Create(config.GetNodeConfig().GetTxHashSetPath().u8string() + "rangeproof/pmmr_leafset.bin", rangeproofBitmap);

		// Rewind RangeProof MMR
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(config.GetNodeConfig().GetTxHashSetPath());
		pRangeProofPMMR->Rewind(pHeader->GetOutputMMRSize(), Roaring());
		pRangeProofPMMR->Commit();

		return std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, pHeader));
	}
	else
	{
		FileUtil::RemoveFile(zipFilePath.u8string());
	}

	return nullptr;
}

bool TxHashSetManager::SaveSnapshot(std::shared_ptr<const IBlockDB> pBlockDB, BlockHeaderPtr pHeader, const std::string& zipFilePath)
{
	if (m_pTxHashSet == nullptr)
	{
		return false;
	}

	fs::path snapshotPath = fs::temp_directory_path() / "Snapshots" / pHeader->ShortHash();
	const std::string snapshotDir = snapshotPath.u8string();
	BlockHeaderPtr pFlushedHeader = nullptr;

	{
		// 1. Lock TxHashSet
		auto reader = m_pTxHashSet->Read();

		// 2. Copy to Snapshots/Hash // TODO: If already exists, just use that.
		if (!FileUtil::CopyDirectory(m_config.GetNodeConfig().GetTxHashSetPath().u8string(), snapshotPath.u8string()))
		{
			return false;
		}

		pFlushedHeader = reader->GetFlushedBlockHeader();
	}

	try
	{
		// 4. Load Snapshot TxHashSet
		std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(snapshotDir);
		std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(snapshotDir);
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(snapshotDir);
		TxHashSet snapshotTxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, pFlushedHeader);

		// 5. Rewind Snapshot TxHashSet
		if (!snapshotTxHashSet.Rewind(pBlockDB, *pHeader))
		{
			FileUtil::RemoveFile(snapshotDir);
			return false;
		}

		// 6. Flush Snapshot TxHashSet
		snapshotTxHashSet.Commit();

		// 7. Rename pmmr_leaf files
		FileUtil::RenameFile(snapshotDir + "output/pmmr_leaf.bin", snapshotDir + "output/pmmr_leaf.bin." + pHeader->ShortHash());
		FileUtil::RenameFile(snapshotDir + "rangeproof/pmmr_leaf.bin", snapshotDir + "rangeproof/pmmr_leaf.bin." + pHeader->ShortHash());

		// 8. Create Zip
		const std::vector<std::string> pathsToZip = { snapshotDir + "kernel", snapshotDir + "output", snapshotDir + "rangeproof" };
		if (!Zipper::CreateZipFile(zipFilePath, pathsToZip))
		{
			FileUtil::RemoveFile(snapshotDir);
			return false;
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to snapshot: {}", e.what());
		FileUtil::RemoveFile(snapshotDir);
		return false;
	}

	// 9. Delete Snapshots/Hash folder
	FileUtil::RemoveFile(snapshotDir);

	return true;
}
