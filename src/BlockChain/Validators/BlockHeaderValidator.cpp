#include "BlockHeaderValidator.h"

#include <Common/Util/HexUtil.h>
#include <Consensus/BlockDifficulty.h>
#include <Consensus/BlockTime.h>
#include <Consensus/HardForks.h>
#include <Infrastructure/Logger.h>
#include <PoW/PoWManager.h>
#include <PMMR/HeaderMMR.h>
#include <chrono>

BlockHeaderValidator::BlockHeaderValidator(
	const Config& config,
	std::shared_ptr<const IBlockDB> pBlockDB,
	std::shared_ptr<const IHeaderMMR> pHeaderMMR)
	: m_config(config), m_pBlockDB(pBlockDB), m_pHeaderMMR(pHeaderMMR)
{

}

bool BlockHeaderValidator::IsValidHeader(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	// Validate Height
	if (header.GetHeight() != (previousHeader.GetHeight() + 1))
	{
		LOG_WARNING_F("Invalid height for header %s", header);
		return false;
	}

	// Validate Timestamp - Ensure timestamp not too far in the future
	if (header.GetTimestamp() > Consensus::GetMaxBlockTime(std::chrono::system_clock::now()))
	{
		LOG_WARNING_F("Timestamp beyond maxBlockTime for header %s", header);
		return false;
	}

	// Validate Version
	const bool validHeaderVersion = Consensus::IsValidHeaderVersion(m_config.GetEnvironment().GetEnvironmentType(), header.GetHeight(), header.GetVersion());
	if (!validHeaderVersion)
	{
		LOG_WARNING_F("Invalid version for header %s", header);
		return false;
	}

	// Validate Timestamp
	if (header.GetTimestamp() <= previousHeader.GetTimestamp())
	{
		LOG_WARNING_F("Timestamp not after previous for header %s", header);
		return false;
	}

	// Validate Proof Of Work
	const bool validPoW = PoWManager(m_config, m_pBlockDB).IsPoWValid(header, previousHeader);
	if (!validPoW)
	{
		LOG_WARNING_F("Invalid Proof of Work for header %s", header);
		return false;
	}

	// Validate the previous header MMR root is correct against the local MMR.
	if (m_pHeaderMMR->Root(header.GetHeight() - 1) != header.GetPreviousRoot())
	{
		LOG_WARNING_F("Invalid Header MMR Root for header %s", header);
		return false;
	}

	LOG_TRACE_F("Header (%s) valid", header);
	return true;
}