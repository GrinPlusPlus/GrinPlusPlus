#include "ShutdownManager.h"

ShutdownManager& ShutdownManager::GetInstance()
{
	static ShutdownManager instance;
	return instance;
}