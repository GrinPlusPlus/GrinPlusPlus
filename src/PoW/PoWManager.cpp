#include <PoW/PoWManager.h>

#include "PoWValidator.h"
#include <Core/Global.h>

PoWManager::PoWManager(const Config& config, std::shared_ptr<const IBlockDB> pBlockDB)
	: m_config(config), m_pBlockDB(pBlockDB)
{

}

bool PoWManager::IsPoWValid(const BlockHeader& header, const BlockHeader& previousHeader) const
{
	if (Global::IsAutomatedTesting())
	{
		return true;
	}

	return PoWValidator(m_config, m_pBlockDB).IsPoWValid(header, previousHeader);
}