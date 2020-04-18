#include <TorProcessManager.h>

TorProcessManager& TorProcessManager::Get() { return test::SingleTon<TorProcessManager>::get(); }