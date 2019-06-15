#include "BlockHeaderValidator.h"

#include <Common/Util/HexUtil.h>
#include <Consensus/BlockDifficulty.h>
#include <Consensus/BlockTime.h>
#include <Consensus/HardForks.h>
#include <Infrastructure/Logger.h>
#include <PoW/PoWManager.h>
#include <PMMR/HeaderMMR.h>
#include <chrono>

BlockHeaderValidator::BlockHeaderValidator(const Config& config, const IBlockDB& blockDB, const IHeaderMMR& headerMMR)
	: m_config(config), m_blockDB(blockDB), m_headerMMR(headerMMR)
{

}

// TODO: Return status enum with error type instead of just true/false
// TODO: Look up previous header instead of taking it in
bool BlockHeaderValidator::IsValidHeader(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	// Validate Height
	if (header.GetHeight() != (previousHeader.GetHeight() + 1))
	{
		LoggerAPI::LogWarning("BlockHeaderValidator::IsValidHeader - Invalid height for header " + HexUtil::ConvertHash(header.GetHash()));
		return false;
	}

	// Validate Timestamp (Refuse blocks more than 12 block intervals in the future.)
	const auto maxBlockTime = std::chrono::system_clock::now() + std::chrono::seconds(12 * Consensus::BLOCK_TIME_SEC);
	if (header.GetTimestamp() > maxBlockTime.time_since_epoch().count())
	{
		LoggerAPI::LogWarning("BlockHeaderValidator::IsValidHeader - Timestamp beyond maxBlockTime for header " + HexUtil::ConvertHash(header.GetHash()));
		return false;
	}

	// Validate Version
	const bool validHeaderVersion = Consensus::IsValidHeaderVersion(m_config.GetEnvironment().GetEnvironmentType(), header.GetHeight(), header.GetVersion());
	if (!validHeaderVersion)
	{
		LoggerAPI::LogWarning("BlockHeaderValidator::IsValidHeader - Invalid version for header " + HexUtil::ConvertHash(header.GetHash()));
		return false;
	}

	// Validate Timestamp
	if (header.GetTimestamp() <= previousHeader.GetTimestamp())
	{
		LoggerAPI::LogWarning("BlockHeaderValidator::IsValidHeader - Timestamp not after previous for header " + HexUtil::ConvertHash(header.GetHash()));
		return false;
	}

	// Validate Proof Of Work
	const bool validPoW = PoWManager(m_config, m_blockDB).IsPoWValid(header, previousHeader);
	if (!validPoW)
	{
		LoggerAPI::LogWarning("BlockHeaderValidator::IsValidHeader - Invalid Proof of Work for header " + HexUtil::ConvertHash(header.GetHash()));
		return false;
	}

	// Validate the previous header MMR root is correct against the local MMR.
	if (m_headerMMR.Root(header.GetHeight() - 1) != header.GetPreviousRoot())
	{
		LoggerAPI::LogWarning("BlockHeaderValidator::IsValidHeader - Invalid Header MMR Root for header " + HexUtil::ConvertHash(header.GetHash()));
		return false;
	}

	LoggerAPI::LogTrace("BlockHeaderValidator::IsValidHeader - Header valid " + HexUtil::ConvertHash(header.GetHash()));
	return true;
}