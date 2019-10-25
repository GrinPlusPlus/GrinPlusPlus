#include "TxHashSetZip.h"
#include "ZipFile.h"

#include <Common/Util/FileUtil.h>
#include <Common/Util/HexUtil.h>
#include <Infrastructure/Logger.h>
#include <filesystem.h>

TxHashSetZip::TxHashSetZip(const Config &config) : m_config(config)
{
}

bool TxHashSetZip::Extract(const std::string &path, const BlockHeader &header) const
{
    ZipFile zipFile(path);
    zipFile.Open();

    std::vector<std::string> files;
    zipFile.ListFiles(files);

    // Extract kernel folder.
    if (!ExtractKernelFolder(zipFile))
    {
        zipFile.Close();
        return false;
    }

    // Extract output folder.
    if (!ExtractOutputFolder(zipFile, header))
    {
        zipFile.Close();
        return false;
    }

    // Extract rangeProof folder.
    if (!ExtractRangeProofFolder(zipFile, header))
    {
        zipFile.Close();
        return false;
    }

    LOG_INFO("Successfully extracted zip file.");
    zipFile.Close();
    return true;
}

bool TxHashSetZip::ExtractKernelFolder(const ZipFile &zipFile) const
{
    const std::string kernelDir = m_config.GetTxHashSetDirectory() + "kernel";
    const fs::path kernelPath(kernelDir);
    if (fs::exists(kernelPath))
    {
        LOG_DEBUG("Kernel folder exists. Deleting its contents now.");
        std::error_code errorCode;
        const uint64_t removedFiles = fs::remove_all(kernelPath, errorCode);
        LOG_DEBUG(std::to_string(removedFiles) + " files removed with error_code " + std::to_string(errorCode.value()));
    }

    const bool kernelDirCreated = fs::create_directories(kernelPath);
    if (!kernelDirCreated)
    {
        LOG_ERROR("Failed to create Kernel folder.");
        return false;
    }

    const std::vector<std::string> kernelFiles = {"pmmr_data.bin", "pmmr_hash.bin"};
    for (const std::string &file : kernelFiles)
    {
        const EZipFileStatus extractStatus = zipFile.ExtractFile("kernel/" + file, kernelDir + "/" + file);
        if (extractStatus != EZipFileStatus::SUCCESS)
        {
            LOG_ERROR("Failed to extract file (" + file + ").");
            return false;
        }
    }

    return true;
}

bool TxHashSetZip::ExtractOutputFolder(const ZipFile &zipFile, const BlockHeader &header) const
{
    const std::string outputDir = m_config.GetTxHashSetDirectory() + "output";
    const fs::path outputPath(outputDir);
    if (fs::exists(outputPath))
    {
        LOG_DEBUG("Output folder exists. Deleting its contents now.");
        std::error_code errorCode;
        const uint64_t removedFiles = fs::remove_all(outputPath, errorCode);
        LOG_DEBUG(std::to_string(removedFiles) + " files removed with error_code " + std::to_string(errorCode.value()));
    }

    const bool outputDirCreated = fs::create_directories(outputPath);
    if (!outputDirCreated)
    {
        LOG_ERROR("Failed to create Output folder.");
        return false;
    }

    const std::vector<std::string> outputFiles = {"pmmr_data.bin", "pmmr_hash.bin", "pmmr_prun.bin",
                                                  "pmmr_leaf.bin." + HexUtil::ConvertHash(header.GetHash())};
    for (const std::string &file : outputFiles)
    {
        const EZipFileStatus extractStatus = zipFile.ExtractFile("output/" + file, outputDir + "/" + file);
        if (extractStatus != EZipFileStatus::SUCCESS)
        {
            LOG_ERROR("Failed to extract file (" + file + ").");
            return false;
        }
    }

    FileUtil::RenameFile(outputDir + "/pmmr_leaf.bin." + HexUtil::ConvertHash(header.GetHash()),
                         outputDir + "/pmmr_leaf.bin");

    return true;
}

bool TxHashSetZip::ExtractRangeProofFolder(const ZipFile &zipFile, const BlockHeader &header) const
{
    const std::string rangeProofDir = m_config.GetTxHashSetDirectory() + "rangeproof";
    const fs::path rangeProofPath(rangeProofDir);
    if (fs::exists(rangeProofPath))
    {
        LOG_DEBUG("RangeProof folder exists. Deleting its contents now.");
        std::error_code errorCode;
        const uint64_t removedFiles = fs::remove_all(rangeProofPath, errorCode);
        LOG_DEBUG(std::to_string(removedFiles) + " files removed with error_code " + std::to_string(errorCode.value()));
    }

    const bool rangeProofDirCreated = fs::create_directories(rangeProofPath);
    if (!rangeProofDirCreated)
    {
        LOG_ERROR("Failed to create RangeProof folder.");
        return false;
    }

    const std::vector<std::string> rangeProofFiles = {"pmmr_data.bin", "pmmr_hash.bin", "pmmr_prun.bin",
                                                      "pmmr_leaf.bin." + HexUtil::ConvertHash(header.GetHash())};
    for (const std::string &file : rangeProofFiles)
    {
        const EZipFileStatus extractStatus = zipFile.ExtractFile("rangeproof/" + file, rangeProofDir + "/" + file);
        if (extractStatus != EZipFileStatus::SUCCESS)
        {
            LOG_ERROR("Failed to extract file (" + file + ").");
            return false;
        }
    }

    FileUtil::RenameFile(rangeProofDir + "/pmmr_leaf.bin." + HexUtil::ConvertHash(header.GetHash()),
                         rangeProofDir + "/pmmr_leaf.bin");

    return true;
}