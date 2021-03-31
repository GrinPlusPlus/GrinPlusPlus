#include "BlockHeaderValidator.h"

#include <Core/Exceptions/BadDataException.h>
#include <Consensus.h>
#include <Common/Util/HexUtil.h>
#include <Common/Logger.h>
#include <PoW/PoWValidator.h>
#include <PMMR/HeaderMMR.h>
#include <chrono>

void BlockHeaderValidator::Validate(const BlockHeader& header, const BlockHeader& prev_header) const
{
    // Validate Height
    if (header.GetHeight() != (prev_header.GetHeight() + 1)) {
        throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlockHeader, "Invalid height for header {}", header);
    }

    // Validate Timestamp - Ensure timestamp not too far in the future
    if (header.GetTimestamp() > Consensus::GetMaxBlockTime(std::chrono::system_clock::now())) {
        throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlockHeader, "Timestamp beyond maxBlockTime for header {}", header);
    }

    // Validate Version
    const uint64_t validHeaderVersion = Consensus::GetHeaderVersion(header.GetHeight());
    if (header.GetVersion() != validHeaderVersion) {
        throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlockHeader, "Invalid version for header {}", header);
    }

    // Validate Timestamp
    if (header.GetTimestamp() <= prev_header.GetTimestamp()) {
        throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlockHeader, "Timestamp not after previous for header {}", header);
    }

    // Validate Proof Of Work
    const bool validPoW = IsPoWValid(header, prev_header);
    if (!validPoW) {
        throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlockHeader, "Invalid Proof of Work for header {}", header);
    }

    // Validate the previous header MMR root is correct against the local MMR.
    if (m_pHeaderMMR->Root(header.GetHeight() - 1) != header.GetPreviousRoot()) {
        throw BAD_DATA_EXCEPTION_F(EBanReason::BadBlockHeader, "Invalid Header MMR Root for header {}", header);
    }

    LOG_TRACE_F("Header {} valid", header);
}

bool BlockHeaderValidator::IsPoWValid(const BlockHeader& header, const BlockHeader& prev_header) const
{
    if (Global::IsAutomatedTesting()) {
        return true;
    }

    return PoWValidator(m_pBlockDB).IsPoWValid(header, prev_header);
}