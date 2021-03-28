#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <Core/Global.h>
#include <Core/Context.h>
#include <TestHelper.h>
#include <TorProcessManager.h>

#include <Common/Logger.h>

TorProcessManager& TorProcessManager::Get() { return test::SingleTon<TorProcessManager>::get(); }

class TestRuntime
{
public:
    TestRuntime()
    {
        m_pConfig = TestHelper::GetTestConfig();
        m_pContext = Context::Create(Environment::AUTOMATED_TESTING, m_pConfig);
        Global::Init(m_pContext);
        LoggerAPI::Initialize(m_pConfig->GetLogDirectory(), m_pConfig->GetLogLevel());
    }

    ~TestRuntime()
    {
        Global::Shutdown();
    }

private:
    ConfigPtr m_pConfig;
    Context::Ptr m_pContext;
};

static TestRuntime RUNTIME;