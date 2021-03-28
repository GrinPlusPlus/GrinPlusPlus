#pragma once

#include <Net/Tor/TorProcess.h>
#include <vector>

// Forward Declarations
class MockTorProcess;

class TorProcessManager
{
public:
    static TorProcess::Ptr GetProcess(const size_t idx);
    static uint16_t LookupPort(const TorAddress& tor_address);

private:
    static TorProcessManager& Get();

    std::vector<std::shared_ptr<MockTorProcess>> m_processes;
};