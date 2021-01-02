#pragma once

#include <Core/Context.h>
#include <Config/Config.h>
#include <atomic>
#include <memory>

// Forward Declarations
class ICoinView;

class Global
{
public:
    /// <summary>
    /// Initializes global context.
    /// </summary>
    /// <param name="pContext"></param>
    static void Init(const Context::Ptr& pContext);
    static const std::atomic_bool& IsRunning();
    static void Shutdown();
    
    static const Config& GetConfig();
    static Context::Ptr GetContext();
    static const Environment& GetEnvVars();
    static EEnvironmentType GetEnv();

    static void SetCoinView(const std::shared_ptr<const ICoinView>& pCoinView);
    static std::shared_ptr<const ICoinView> GetCoinView();

private:
    static Context::Ptr LockContext();
};