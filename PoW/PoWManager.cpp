#include <PoW/PoWManager.h>

#include "PoWValidator.h"

PoWManager::PoWManager(const Config& config)
	: m_config(config)
{

}

bool PoWManager::IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	return PoWValidator(m_config).IsPoWValid(header, previousHeader);
}