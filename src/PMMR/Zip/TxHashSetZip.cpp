#include "TxHashSetZip.h"
#include "ZipFile.h"

#include <Common/Util/HexUtil.h>
#include <Common/Util/FileUtil.h>
#include <Core/Exceptions/FileException.h>
#include <Common/Logger.h>
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

		// Extract kernel folder.
		ExtractKernelFolder(*pZipFile);

		// Extract output folder.
		ExtractFolder(*pZipFile, "output", header);

		// Extract rangeProof folder.
		ExtractFolder(*pZipFile, "rangeproof", header);

		LOG_INFO("Successfully extracted zip file.");
		return true;
	}
	catch (std::exception& e)
	{
		LOG_ERROR_F("Failed to extract {}. Exception thrown: {}", path, e.what());
	}

	return false;
}

void TxHashSetZip::ExtractKernelFolder(const ZipFile& zipFile) const
{
	std::error_code ec;

	const fs::path kernelPath = m_config.GetTxHashSetPath() / "kernel";
	const bool exists = fs::exists(kernelPath, ec);
	if (ec)
	{
		LOG_ERROR_F("fs::exists failed with error: {}", ec.message());
		throw FILE_EXCEPTION_F("fs::exists failed with error: {}", ec.message());
	}

	if (exists)
	{
		LOG_DEBUG("Kernel folder exists. Deleting its contents now.");
		const uint64_t removedFiles = fs::remove_all(kernelPath, ec);
		if (ec)
		{
			LOG_ERROR_F("fs::remove_all failed with error: {}", ec.message());
			throw FILE_EXCEPTION_F("fs::remove_all failed with error: {}", ec.message());
		}

		LOG_DEBUG_F("{} files removed", removedFiles);
	}

	const bool kernelDirCreated = fs::create_directories(kernelPath, ec);
	if (!kernelDirCreated || ec)
	{
		LOG_ERROR_F("Failed to create Kernel folder. Error: {}", ec.message());
		throw FILE_EXCEPTION_F("Failed to create Kernel folder. Error: {}", ec.message());
	}

	const std::vector<std::string> kernelFiles = { "pmmr_data.bin", "pmmr_hash.bin" };
	for (const std::string& file : kernelFiles)
	{
		zipFile.ExtractFile("kernel/" + file, kernelPath / file);
	}
}

void TxHashSetZip::ExtractFolder(const ZipFile& zipFile, const std::string& folderName, const BlockHeader& header) const
{
	std::error_code ec;

	const fs::path dir = m_config.GetTxHashSetPath() / folderName;
	const bool exists = fs::exists(dir, ec);
	if (ec)
	{
		LOG_ERROR_F("fs::exists failed with error: {}", ec.message());
		throw FILE_EXCEPTION_F("fs::exists failed with error: {}", ec.message());
	}

	if (exists)
	{
		LOG_DEBUG_F("{} already exists. Deleting its contents now.", dir);
		const uint64_t removedFiles = fs::remove_all(dir, ec);
		if (ec)
		{
			LOG_ERROR_F("fs::remove_all failed with error: {}", ec.message());
			throw FILE_EXCEPTION_F("fs::remove_all failed with error: {}", ec.message());
		}

		LOG_DEBUG_F("{} files removed", removedFiles);
	}

	const bool dirCreated = fs::create_directories(dir, ec);
	if (!dirCreated || ec)
	{
		LOG_ERROR_F("Failed to create {}. Error: {}", dir, ec.message());
		throw FILE_EXCEPTION_F("Failed to create {}. Error: {}", dir, ec.message());
	}

	const std::vector<std::string> files = { "pmmr_data.bin", "pmmr_hash.bin", "pmmr_prun.bin", "pmmr_leaf.bin." + header.ShortHash() };
	for (const std::string& file : files)
	{
		zipFile.ExtractFile(StringUtil::Format("{}/{}", folderName, file), dir / file);
	}

	FileUtil::RenameFile(dir / StringUtil::Format("pmmr_leaf.bin.{}", header.ShortHash()), dir / "pmmr_leaf.bin");
}