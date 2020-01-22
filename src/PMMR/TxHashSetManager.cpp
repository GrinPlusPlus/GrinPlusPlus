#include <PMMR/TxHashSetManager.h>

#include "TxHashSetImpl.h"
#include "Zip/TxHashSetZip.h"
#include "Zip/Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Infrastructure/Logger.h>

#include <filesystem.h>

TxHashSetManager::TxHashSetManager(const Config& config)
	: m_config(config.GetNodeConfig()), m_pTxHashSet(nullptr)
{

}

std::shared_ptr<Locked<ITxHashSet>> TxHashSetManager::Open(BlockHeaderPtr pConfirmedTip)
{
	Close();

	std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(m_config.GetTxHashSetPath());
	std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(m_config.GetTxHashSetPath());
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(m_config.GetTxHashSetPath());

	auto pTxHashSet = std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, pConfirmedTip));
	m_pTxHashSet = std::make_shared<Locked<ITxHashSet>>(Locked<ITxHashSet>(pTxHashSet));

	return m_pTxHashSet;
}

std::shared_ptr<ITxHashSet> TxHashSetManager::LoadFromZip(const Config& config, const fs::path& zipFilePath, BlockHeaderPtr pHeader)
{
	const fs::path& txHashSetPath = config.GetNodeConfig().GetTxHashSetPath();
	const TxHashSetZip zip(config);
	if (zip.Extract(zipFilePath, *pHeader))
	{
		LOG_INFO_F("{} extracted successfully", zipFilePath);
		FileUtil::RemoveFile(zipFilePath);

		// Rewind Kernel MMR
		std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(txHashSetPath);
		pKernelMMR->Rewind(pHeader->GetKernelMMRSize());
		pKernelMMR->Commit();

		// Create output BitmapFile from Roaring file
		const fs::path leafPath = txHashSetPath / "output" / "pmmr_leaf.bin";
		Roaring outputBitmap;
		std::vector<unsigned char> outputBytes;
		if (FileUtil::ReadFile(leafPath, outputBytes))
		{
			outputBitmap = Roaring::readSafe((const char*)outputBytes.data(), outputBytes.size());
		}
		BitmapFile::Create(txHashSetPath / "output" / "pmmr_leafset.bin", outputBitmap);

		// Rewind Output MMR
		std::shared_ptr<OutputPMMR> pOutputPMMR = OutputPMMR::Load(txHashSetPath);
		pOutputPMMR->Rewind(pHeader->GetOutputMMRSize(), Roaring());
		pOutputPMMR->Commit();

		// Create rangeproof BitmapFile from Roaring file
		const fs::path rangeproofPath = txHashSetPath / "rangeproof" / "pmmr_leaf.bin";
		Roaring rangeproofBitmap;
		std::vector<unsigned char> rangeproofBytes;
		if (FileUtil::ReadFile(rangeproofPath, rangeproofBytes))
		{
			rangeproofBitmap = Roaring::readSafe((const char*)rangeproofBytes.data(), rangeproofBytes.size());
		}
		BitmapFile::Create(txHashSetPath / "rangeproof" / "pmmr_leafset.bin", rangeproofBitmap);

		// Rewind RangeProof MMR
		std::shared_ptr<RangeProofPMMR> pRangeProofPMMR = RangeProofPMMR::Load(txHashSetPath);
		pRangeProofPMMR->Rewind(pHeader->GetOutputMMRSize(), Roaring());
		pRangeProofPMMR->Commit();

		return std::shared_ptr<TxHashSet>(new TxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, pHeader));
	}
	else
	{
		FileUtil::RemoveFile(zipFilePath);
	}

	return nullptr;
}

fs::path TxHashSetManager::SaveSnapshot(std::shared_ptr<const IBlockDB> pBlockDB, BlockHeaderPtr pHeader)
{
	if (m_pTxHashSet == nullptr)
	{
		throw std::exception();
	}

	fs::path snapshotDir = fs::temp_directory_path() / "Snapshots" / pHeader->ShortHash();
	BlockHeaderPtr pFlushedHeader = nullptr;

	{
		// 1. Lock TxHashSet
		auto reader = m_pTxHashSet->Read();

		// 2. Copy to Snapshots/Hash // TODO: If already exists, just use that.
		FileUtil::CopyDirectory(m_config.GetTxHashSetPath(), snapshotDir);

		pFlushedHeader = reader->GetFlushedBlockHeader();
	}

	const std::string fileName = StringUtil::Format("TxHashSet.{}.zip", pHeader->ShortHash());
	fs::path zipFilePath = fs::temp_directory_path() / "Snapshots" / fileName;

	try
	{
		// 4. Load Snapshot TxHashSet
		auto pKernelMMR = KernelMMR::Load(snapshotDir);
		auto pOutputPMMR = OutputPMMR::Load(snapshotDir);
		auto pRangeProofPMMR = RangeProofPMMR::Load(snapshotDir);
		TxHashSet snapshotTxHashSet(pKernelMMR, pOutputPMMR, pRangeProofPMMR, pFlushedHeader);

		// 5. Rewind Snapshot TxHashSet
		snapshotTxHashSet.Rewind(pBlockDB, *pHeader);

		// 6. Flush Snapshot TxHashSet
		snapshotTxHashSet.Commit();

		// 7. Rename pmmr_leaf files
		const std::string newFileName = StringUtil::Format("pmmr_leaf.bin.{}", pHeader->ShortHash());
		FileUtil::RenameFile(
			snapshotDir / "output" / "pmmr_leaf.bin",
			snapshotDir / "output" / newFileName
		);

		FileUtil::RenameFile(
			snapshotDir / "rangeproof" / "pmmr_leaf.bin",
			snapshotDir / "rangeproof" / newFileName
		);

		// 8. Create Zip
		const std::vector<fs::path> pathsToZip = {
			snapshotDir / "kernel",
			snapshotDir / "output",
			snapshotDir / "rangeproof"
		};
		Zipper::CreateZipFile(zipFilePath, pathsToZip);

		// 9. Delete Snapshots/Hash folder
		FileUtil::RemoveFile(snapshotDir);
	}
	catch (std::exception&)
	{
		FileUtil::RemoveFile(snapshotDir);
		throw;
	}

	return zipFilePath;
}
