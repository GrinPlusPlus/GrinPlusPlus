#pragma once

#include <Net/Tor/TorProcess.h>
#include <TestSingleton.h>
#include <vector>

class TorProcessManager
{
public:
    static TorProcess::Ptr GetProcess(const size_t idx)
    {
        TorProcessManager& manager = Get();

        while (manager.m_processes.size() < (idx + 1))
        {
            const uint64_t size = manager.m_processes.size();
            const size_t socksPort = manager.m_processes.empty() ? 3422 : 20000 + (size * 2);
            const size_t controlPort = manager.m_processes.empty() ? 3423 : 20000 + (size * 2) + 1;
            manager.m_processes.push_back(
                TorProcess::Initialize(fs::temp_directory_path() / "grinpp_tor", (uint16_t)socksPort, (uint16_t)controlPort)
            );
        }

        return manager.m_processes[idx];
    }

private:
    static TorProcessManager& Get();

    std::vector<TorProcess::Ptr> m_processes;
};