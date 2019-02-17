#pragma once

#include <Core/Models/BlockHeader.h>
#include <Config/Config.h>
#include <string>

// Forward Declarations
class ZipFile;

class TxHashSetZip
{
public:
	TxHashSetZip(const Config& config);

	bool Extract(const std::string& path, const BlockHeader& header) const;

private:
	bool ExtractKernelFolder(const ZipFile& zipFile) const;
	bool ExtractOutputFolder(const ZipFile& zipFile, const BlockHeader& header) const;
	bool ExtractRangeProofFolder(const ZipFile& zipFile, const BlockHeader& header) const;

	const Config& m_config;
};