#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include <Core/Global.h>
#include <Core/Context.h>
#include <TestHelper.h>

#include <Common/Logger.h>
#include <iostream>

class TestRuntime
{
public:
    TestRuntime()
    {
        m_pConfig = TestHelper::GetTestConfig();
        m_pContext = Context::Create(Environment::AUTOMATED_TESTING, m_pConfig);
        Global::Init(m_pContext);
        LoggerAPI::Initialize(m_pConfig->GetLogDirectory(), m_pConfig->GetLogLevel());

        std::cout
            << "Test Runtime Initialized." << std::endl
            << "Running Tests..." << std::endl;
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