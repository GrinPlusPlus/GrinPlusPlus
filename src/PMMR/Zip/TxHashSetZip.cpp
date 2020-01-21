#include "TxHashSetZip.h"
#include "ZipFile.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Infrastructure/Logger.h>
#include <filesystem.h>

TxHashSetZip::TxHashSetZip(const Config& config)
	: m_config(config)
{

}

bool TxHashSetZip::Extract(const fs::path& path, const BlockHeader& header) const
{
	try
	{
		std::shared_ptr<ZipFile> pZipFile = ZipFile::Load(path);

		std::vector<std::string> files = pZipFile->ListFiles();

		// Extract kernel folder.
		if (!ExtractKernelFolder(*pZipFile))
		{
			return false;
		}

		// Extract output folder.
		if (!ExtractOutputFolder(*pZipFile, header))
		{
			return false;
		}

		// Extract rangeProof folder.
		if (!ExtractRangeProofFolder(*pZipFile, header))
		{
			return false;
		}

		LOG_INFO("Successfully extracted zip file.");
		return true;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to extract {}. Exception thrown: {}", path, e.what());
	}

	return false;
}

bool TxHashSetZip::ExtractKernelFolder(const ZipFile& zipFile) const
{
	const fs::path kernelPath = m_config.GetNodeConfig().GetTxHashSetPath() / "kernel";
	if (fs::exists(kernelPath))
	{
		LOG_DEBUG("Kernel folder exists. Deleting its contents now.");
		std::error_code errorCode;
		const uint64_t removedFiles = fs::remove_all(kernelPath, errorCode);
		LOG_DEBUG_F("{} files removed with error_code {}", removedFiles, errorCode.value());
	}

	const bool kernelDirCreated = fs::create_directories(kernelPath);
	if (!kernelDirCreated)
	{
		LOG_ERROR("Failed to create Kernel folder.");
		return false;
	}

	const std::vector<std::string> kernelFiles = { "pmmr_data.bin", "pmmr_hash.bin" };
	for (const std::string& file : kernelFiles)
	{
		zipFile.ExtractFile("kernel/" + file, kernelPath / file);
	}

	return true;
}

bool TxHashSetZip::ExtractOutputFolder(const ZipFile& zipFile, const BlockHeader& header) const
{
	const fs::path outputDir = m_config.GetNodeConfig().GetTxHashSetPath() / "output";
	if (fs::exists(outputDir))
	{
		LOG_DEBUG("Output folder exists. Deleting its contents now.");
		std::error_code errorCode;
		const uint64_t removedFiles = fs::remove_all(outputDir, errorCode);
		LOG_DEBUG(std::to_string(removedFiles) + " files removed with error_code " + std::to_string(errorCode.value()));
	}

	const bool outputDirCreated = fs::create_directories(outputDir);
	if (!outputDirCreated)
	{
		LOG_ERROR("Failed to create Output folder.");
		return false;
	}

	const std::vector<std::string> outputFiles = { "pmmr_data.bin", "pmmr_hash.bin", "pmmr_prun.bin", "pmmr_leaf.bin." + header.ShortHash() };
	for (const std::string& file : outputFiles)
	{
		zipFile.ExtractFile("output/" + file, outputDir / file);
	}

	FileUtil::RenameFile(outputDir / StringUtil::Format("pmmr_leaf.bin.{}", header.ShortHash()), outputDir / "pmmr_leaf.bin");

	return true;
}

bool TxHashSetZip::ExtractRangeProofFolder(const ZipFile& zipFile, const BlockHeader& header) const
{
	const fs::path rangeProofDir = m_config.GetNodeConfig().GetTxHashSetPath() / "rangeproof";
	if (fs::exists(rangeProofDir))
	{
		LOG_DEBUG("RangeProof folder exists. Deleting its contents now.");
		std::error_code errorCode;
		const uint64_t removedFiles = fs::remove_all(rangeProofDir, errorCode);
		LOG_DEBUG_F("{} files removed with error_code {}", removedFiles, errorCode.value());
	}

	const bool rangeProofDirCreated = fs::create_directories(rangeProofDir);
	if (!rangeProofDirCreated)
	{
		LOG_ERROR("Failed to create RangeProof folder.");
		return false;
	}

	const std::vector<std::string> rangeProofFiles = { "pmmr_data.bin", "pmmr_hash.bin", "pmmr_prun.bin", "pmmr_leaf.bin." + header.ShortHash() };
	for (const std::string& file : rangeProofFiles)
	{
		zipFile.ExtractFile("rangeproof/" + file, rangeProofDir / file);
	}

	FileUtil::RenameFile(rangeProofDir / StringUtil::Format("pmmr_leaf.bin.{}", header.ShortHash()), rangeProofDir / "pmmr_leaf.bin");

	return true;
}