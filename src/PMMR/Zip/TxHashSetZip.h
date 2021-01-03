#pragma once

#include <Core/Models/BlockHeader.h>
#include <Core/Config.h>
#include <filesystem.h>

// Forward Declarations
class ZipFile;

class TxHashSetZip
{
public:
	TxHashSetZip(const Config& config);

	bool Extract(const fs::path& path, const BlockHeader& header) const;

private:
	void ExtractKernelFolder(const ZipFile& zipFile) const;
	void ExtractFolder(const ZipFile& zipFile, const std::string& folderName, const BlockHeader& header) const;
	void ExtractOutputFolder(const ZipFile& zipFile, const BlockHeader& header) const;
	void ExtractRangeProofFolder(const ZipFile& zipFile, const BlockHeader& header) const;

	const Config& m_config;
};