#include <PMMR/TxHashSetManager.h>

#include "TxHashSetImpl.h"
#include "Zip/TxHashSetZip.h"
#include "Zip/Zipper.h"

#include <Common/Util/FileUtil.h>
#include <Common/Util/StringUtil.h>
#include <Core/File/FileRemover.h>
#include <Common/Logger.h>

#include <filesystem.h>
#include <thread>

TxHashSetManager::TxHashSetManager(const Config& config)
	: m_config(config), m_pTxHashSet(nullptr)
{

}

std::shared_ptr<ITxHashSet> TxHashSetManager::Open(const BlockHeaderPtr& pConfirmedTip)
{
	Close();

	std::shared_ptr<KernelMMR> pKernelMMR;
	std::shared_ptr<OutputPMMR> pOutputPMMR;
	std::shared_ptr<RangeProofPMMR> pRangeProofPMMR;

	std::vector<std::thread> threads;
	threads.emplace_back(std::thread([this, &pKernelMMR] {
		pKernelMMR = KernelMMR::Load(Global::GetConfig().GetTxHashSetPath());
	}));
	threads.emplace_back(std::thread([this, &pOutputPMMR] {
		pOutputPMMR = OutputPMMR::Load(Global::GetConfig().GetTxHashSetPath());
	}));
	threads.emplace_back(std::thread([this, &pRangeProofPMMR] {
		pRangeProofPMMR = RangeProofPMMR::Load(Global::GetConfig().GetTxHashSetPath());
	}));

	for (auto& thread : threads) {
		if (thread.joinable()) {
			thread.join();
		}
	}

	m_pTxHashSet = std::shared_ptr<TxHashSet>(new TxHashSet(m_config, pKernelMMR, pOutputPMMR, pRangeProofPMMR, pConfirmedTip));

	return m_pTxHashSet;
}

std::shared_ptr<ITxHashSet> TxHashSetManager::LoadFromZip(const Config& config, const fs::path& zipFilePath, BlockHeaderPtr pHeader)
{
	FileRemover fileRemover(zipFilePath);

	const fs::path& txHashSetPath = config.GetTxHashSetPath();
	const TxHashSetZip zip(config);

	try
	{
		if (zip.Extract(zipFilePath, *pHeader))
		{
			LOG_INFO_F("{} extracted successfully", zipFilePath);
			FileUtil::RemoveFile(zipFilePath);

			// Rewind Kernel MMR
			std::shared_ptr<KernelMMR> pKernelMMR = KernelMMR::Load(txHashSetPath);
			pKernelMMR->Rewind(pHeader->GetNumKernels());
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
			pOutputPMMR->Rewind(pHeader->GetNumOutputs(), {});
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
			pRangeProofPMMR->Rewind(pHeader->GetNumOutputs(), {});
			pRangeProofPMMR->Commit();

			return std::shared_ptr<TxHashSet>(new TxHashSet(config, pKernelMMR, pOutputPMMR, pRangeProofPMMR, pHeader));
		}
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to load: {}", e.what());
	}

	return nullptr;
}

fs::path TxHashSetManager::SaveSnapshot(std::shared_ptr<IBlockDB> pBlockDB, BlockHeaderPtr pHeader) const
{
	if (m_pTxHashSet == nullptr)
	{
		throw std::exception();
	}

	fs::path snapshotDir = fs::temp_directory_path() / "Snapshots" / pHeader->ShortHash();
	FileRemover snapshotRemover(snapshotDir);

	const std::string fileName = StringUtil::Format("TxHashSet.{}.zip", pHeader->ShortHash());
	fs::path zipFilePath = fs::temp_directory_path() / "Snapshots" / fileName;

	try
	{
		BlockHeaderPtr pFlushedHeader = nullptr;

		{
			// Copy to Snapshots/Hash // TODO: If already exists, just use that.
			FileUtil::CopyDirectory(m_config.GetTxHashSetPath(), snapshotDir);

			pFlushedHeader = m_pTxHashSet->GetFlushedBlockHeader();
		}

		{
			// Load Snapshot TxHashSet
			auto pKernelMMR = KernelMMR::Load(snapshotDir);
			auto pOutputPMMR = OutputPMMR::Load(snapshotDir);
			auto pRangeProofPMMR = RangeProofPMMR::Load(snapshotDir);
			TxHashSet snapshotTxHashSet(m_config, pKernelMMR, pOutputPMMR, pRangeProofPMMR, pFlushedHeader);

			// Rewind Snapshot TxHashSet
			snapshotTxHashSet.Rewind(pBlockDB, *pHeader);

			// Flush Snapshot TxHashSet
			snapshotTxHashSet.Commit();
		}

		// Rename pmmr_leaf files
		const std::string newFileName = StringUtil::Format("pmmr_leaf.bin.{}", pHeader->ShortHash());
		FileUtil::RenameFile(
			snapshotDir / "output" / "pmmr_leaf.bin",
			snapshotDir / "output" / newFileName
		);

		FileUtil::RenameFile(
			snapshotDir / "rangeproof" / "pmmr_leaf.bin",
			snapshotDir / "rangeproof" / newFileName
		);

		// Create Zip
		const std::vector<fs::path> pathsToZip = {
			snapshotDir / "kernel",
			snapshotDir / "output",
			snapshotDir / "rangeproof"
		};
		Zipper::CreateZipFile(zipFilePath, pathsToZip);
	}
	catch (...)
	{
		FileUtil::RemoveFile(zipFilePath);
		throw;
	}

	return zipFilePath;
}
