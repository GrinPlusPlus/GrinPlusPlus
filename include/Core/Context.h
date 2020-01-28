#pragma once

#include <Config/Config.h>
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

        return std::shared_ptr<Context>(new Context(pConfig, std::make_shared<Bosma::Scheduler>(12)));
    }

    const Config& GetConfig() const { return *m_pConfig; }
    std::shared_ptr<Bosma::Scheduler> GetScheduler() noexcept { return m_pScheduler; }

private:
    // TODO: Include logger
    Context(const ConfigPtr& pConfig, const std::shared_ptr<Bosma::Scheduler>& pScheduler)
        : m_pConfig(pConfig), m_pScheduler(pScheduler) { }

    ConfigPtr m_pConfig;
    std::shared_ptr<Bosma::Scheduler> m_pScheduler;
};