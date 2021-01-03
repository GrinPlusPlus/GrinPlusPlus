#pragma once

#include <Config/EnvironmentType.h>
#include <atomic>
#include <memory>

// Forward Declarations
class Context;
class Config;
class ICoinView;
class Environment;

class Global
{
public:
    /// <summary>
    /// Initializes global context.
    /// </summary>
    /// <param name="pContext"></param>
    static void Init(const std::shared_ptr<Context>& pContext);
    static const std::atomic_bool& IsRunning();
    static void Shutdown();
    
    static const Config& GetConfig();
    static std::shared_ptr<Context> GetContext();
    static const Environment& GetEnvVars();
    static EEnvironmentType GetEnv();

    static void SetCoinView(const std::shared_ptr<const ICoinView>& pCoinView);
    static std::shared_ptr<const ICoinView> GetCoinView();

private:
    static std::shared_ptr<Context> LockContext();
};