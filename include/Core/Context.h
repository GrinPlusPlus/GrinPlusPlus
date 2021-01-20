#pragma once

#include <Core/Config.h>
#include <Net/Tor/TorProcess.h>
#include <scheduler/Scheduler.h>
#include <Common/Logger.h>
#include <memory>
#include <cassert>

class Context
{
public:
    using Ptr = std::shared_ptr<Context>;

    Context(
        const Environment env,
        const ConfigPtr& pConfig,
        const std::shared_ptr<Bosma::Scheduler>& pScheduler
    ) : m_env(env), m_pConfig(pConfig), m_pScheduler(pScheduler) { }

    ~Context() {
        LOG_INFO("Deleting node context");
        m_pScheduler.reset();
        m_pConfig.reset();
    }

    static Context::Ptr Create(const Environment env, const ConfigPtr& pConfig)
    {
        assert(pConfig != nullptr);

        return std::make_shared<Context>(
            env,
            pConfig,
            std::make_shared<Bosma::Scheduler>(12)
        );
    }

    Environment GetEnvironment() const noexcept { return m_env; }
    Config& GetConfig() noexcept { return *m_pConfig; }
    const Config& GetConfig() const noexcept { return *m_pConfig; }
    const std::shared_ptr<Bosma::Scheduler>& GetScheduler() const noexcept { return m_pScheduler; }

private:
    // TODO: Include logger
    Environment m_env;
    ConfigPtr m_pConfig;
    std::shared_ptr<Bosma::Scheduler> m_pScheduler;
};