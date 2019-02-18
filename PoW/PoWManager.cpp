#include <PoW/PoWManager.h>

#include "PoWValidator.h"

PoWManager::PoWManager(const Config& config, const IBlockDB& blockDB)
	: m_config(config), m_blockDB(blockDB)
{

}

bool PoWManager::IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	return PoWValidator(m_config, m_blockDB).IsPoWValid(header, previousHeader);
}