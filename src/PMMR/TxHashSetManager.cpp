#include <PMMR/TxHashSetManager.h>

#include "TxHashSetImpl.h"
#include "Zip/TxHashSetZip.h"
#include "Zip/Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

TxHashSetManager::TxHashSetManager(const Config& config)
	: m_config(config), m_pTxHashSet(nullptr)
{

}

std::shared_ptr<Locked<ITxHashSet>> TxHashSetManager::Open(const BlockHeader& confirmedTip)
{
	Close();

	std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(m_config.GetTxHashSetDirectory());
	std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(m_config.GetTxHashSetDirectory());
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(m_config.GetTxHashSetDirectory());

	auto pTxHashSet = std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, confirmedTip));
	m_pTxHashSet = std::make_shared<Locked<ITxHashSet>>(Locked<ITxHashSet>(pTxHashSet));

	return m_pTxHashSet;
}

std::shared_ptr<ITxHashSet> TxHashSetManager::LoadFromZip(const Config& config, const fs::path& zipFilePath, const BlockHeader& blockHeader)
{
	const TxHashSetZip zip(config);
	if (zip.Extract(zipFilePath, blockHeader))
	{
		LOG_INFO_F("%s extracted successfully", zipFilePath);
		FileUtil::RemoveFile(zipFilePath.u8string());

		// Rewind Kernel MMR
		std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(config.GetTxHashSetDirectory());
		pKernelMMR->Rewind(blockHeader.GetKernelMMRSize());
		pKernelMMR->Commit();

		// Create output BitmapFile from Roaring file
		const std::string leafPath = config.GetTxHashSetDirectory().u8string() + "output/pmmr_leaf.bin";
		Roaring outputBitmap;
		std::vector<unsigned char> outputBytes;
		if (FileUtil::ReadFile(leafPath, outputBytes))
		{
			outputBitmap = Roaring::readSafe((const char*)outputBytes.data(), outputBytes.size());
		}
		BitmapFile::Create(config.GetTxHashSetDirectory().u8string() + "output/pmmr_leafset.bin", outputBitmap);

		// Rewind Output MMR
		std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(config.GetTxHashSetDirectory());
		pOutputPMMR->Rewind(blockHeader.GetOutputMMRSize(), Roaring());
		pOutputPMMR->Commit();

		// Create rangeproof BitmapFile from Roaring file
		const std::string rangeproofPath = config.GetTxHashSetDirectory().u8string() + "rangeproof/pmmr_leaf.bin";
		Roaring rangeproofBitmap;
		std::vector<unsigned char> rangeproofBytes;
		if (FileUtil::ReadFile(rangeproofPath, rangeproofBytes))
		{
			rangeproofBitmap = Roaring::readSafe((const char*)rangeproofBytes.data(), rangeproofBytes.size());
		}
		BitmapFile::Create(config.GetTxHashSetDirectory().u8string() + "rangeproof/pmmr_leafset.bin", rangeproofBitmap);

		// Rewind RangeProof MMR
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(config.GetTxHashSetDirectory());
		pRangeProofPMMR->Rewind(blockHeader.GetOutputMMRSize(), Roaring());
		pRangeProofPMMR->Commit();

		return std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, blockHeader));
	}

	return nullptr;
}

bool TxHashSetManager::SaveSnapshot(std::shared_ptr<const IBlockDB> pBlockDB, const BlockHeader& blockHeader, const std::string& zipFilePath)
{
	if (m_pTxHashSet == nullptr)
	{
		return false;
	}

	const std::string snapshotDir = StringUtil::Format("%sSnapshots/%s/", fs::temp_directory_path().string(), blockHeader.ShortHash());
	std::unique_ptr<BlockHeader> pFlushedHeader = nullptr;

	{
		// 1. Lock TxHashSet
		auto reader = m_pTxHashSet->Read();

		// 2. Copy to Snapshots/Hash // TODO: If already exists, just use that.
		if (!FileUtil::CopyDirectory(m_config.GetTxHashSetDirectory().u8string(), snapshotDir))
		{
			return false;
		}

		pFlushedHeader = std::make_unique<BlockHeader>(BlockHeader(reader->GetFlushedBlockHeader()));
	}

	{
		// 4. Load Snapshot TxHashSet
		std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(snapshotDir);
		std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(snapshotDir);
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(snapshotDir);
		TxHashSet snapshotTxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, *pFlushedHeader);

		// 5. Rewind Snapshot TxHashSet
		if (!snapshotTxHashSet.Rewind(pBlockDB, blockHeader))
		{
			return false;
		}

		// 6. Flush Snapshot TxHashSet
		snapshotTxHashSet.Commit();

		// 7. Rename pmmr_leaf files
		FileUtil::RenameFile(snapshotDir + "output/pmmr_leaf.bin", snapshotDir + "output/pmmr_leaf.bin." + blockHeader.ShortHash());
		FileUtil::RenameFile(snapshotDir + "rangeproof/pmmr_leaf.bin", snapshotDir + "rangeproof/pmmr_leaf.bin." + blockHeader.ShortHash());

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