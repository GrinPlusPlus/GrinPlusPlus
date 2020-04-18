#pragma once

#include <Config/Config.h>
#include <Net/Tor/TorProcess.h>
#include <scheduler/Scheduler.h>
#include <memory>
#include <cassert>

class Context
{
public:
    using Ptr = std::shared_ptr<Context>;

    static Context::Ptr Create(const ConfigPtr& pConfig)
    {
        assert(pConfig != nullptr);

        auto pTorProcess = TorProcess::Initialize(
            pConfig->GetTorConfig().GetSocksPort(),
            pConfig->GetTorConfig().GetControlPort()
        );
        return std::shared_ptr<Context>(new Context(
            pConfig,
            std::make_shared<Bosma::Scheduler>(12),
            pTorProcess
        ));
    }

    const Config& GetConfig() const { return *m_pConfig; }
    const std::shared_ptr<Bosma::Scheduler>& GetScheduler() const noexcept { return m_pScheduler; }
    const TorProcess::Ptr& GetTorProcess() const noexcept { return m_pTorProcess; }

private:
    // TODO: Include logger
    Context(
        const ConfigPtr& pConfig,
        const std::shared_ptr<Bosma::Scheduler>& pScheduler,
        const TorProcess::Ptr& pTorProcess
    ) : m_pConfig(pConfig), m_pScheduler(pScheduler), m_pTorProcess(pTorProcess) { }

    ConfigPtr m_pConfig;
    std::shared_ptr<Bosma::Scheduler> m_pScheduler;
    TorProcess::Ptr m_pTorProcess;
};